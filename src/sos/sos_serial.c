#include <stddef.h>
#include <string.h>
#include <serial.h>
#include <assert.h>

#include "sos_serial.h"
#include "l4.h"
#include "libsos.h"

#define verbose 1
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define READBUF_SIZE 256
#define IPC_MAX_WORDS 64

// struct used to communicate with the serial driver
struct serial* ser = NULL;

// buffer for incoming chars
char buf[READBUF_SIZE];
unsigned int bufwrite = 0;
unsigned int bufread = 0;

/**
 * Initializes serial struct and registers handler function
 * function to receive input data.
 *
 */
void sos_serial_init(void) {
	dprintf(0,"sos_serial_init\n");
	ser = serial_init();
	serial_register_handler(ser, &sos_serial_receive);
	assert(ser != NULL);
}

/**
 * Writes incoming IPC strings out to the serial port.
 *
 * The Structure of the IPC message should be as follows
 * 1st Word: Length of the string
 * 2nd-5th Word: Message String
 *
 * @param msg_p pointer to the IPC message
 *
 */
void sos_serial_send(L4_Msg_t* msg_p) {
	dprintf(1, "received SOS_SERIAL_WRITE\n");

	// serial struct must be initialized
	assert(ser != NULL);

	int total_sent = 0;
	int sent = 0;
	int len = L4_MsgWord(msg_p, 0);

	L4_Word_t write_buffer[4];

	dprintf(2, "len is: %d\n", len);

	// copy in buffer here
	int i;
	for(i=1; i<=4; i++) {
		write_buffer[i-1] = L4_MsgWord(msg_p, i);
	}

	dprintf(2, "message is: %.16s\n", &write_buffer);
	char* message_ptr = (char*) &write_buffer;

	// sending to serial
	for (int i=0; i < 20; i++) {
		sent = serial_send(ser, message_ptr, len-total_sent);
		message_ptr += sent;

		total_sent += sent;

		// In case we send faster than our serial driver allows we
		// retry until all data is sent or at most 20 times.
		if(total_sent < len) {
			dprintf(0, "sos_serial_send: serial driver's internal buffer fills faster than it can actually output data");
		}
		else {
			// everything sent, can exit loop
			break;
		}
	}

	// send the number of written chars back to the user thread
    L4_MsgClear(msg_p);
    L4_MsgAppendWord(msg_p, total_sent);
    L4_MsgLoad(msg_p);
}

/**
 * Adds incoming chars from the serial port to the circular buffer.
 *
 * @param serial pointer to the serial struct
 * @param c received char
 *
 */
void sos_serial_receive (struct serial *serial, char c) {
	// discard char if buffer is full
	dprintf(0,"%c",c);
	if ((bufwrite+1) % READBUF_SIZE != bufread) {
		buf[bufwrite] = c;
		bufwrite = (bufwrite+1) % READBUF_SIZE;
	}
}

/**
 * Reads serial buffer and constructs IPC message to be
 * sent to the reading user thread.
 *
 * The message in msg_p must contain one word with the
 * desired number of chars to read from the serial port.
 *
 * The Structure of the generated IPC message is as follows
 * 1st Word: Length of the string
 * 2nd-64th Word: Message String
 *
 * @param msg_p pointer to the IPC message
 *
 */
void sos_serial_read (L4_Msg_t* msg_p) {
	dprintf(1, "received SOS_SERIAL_READ\n");

	// how many chars to write? read from message and check against buffer fill
	size_t to_send = (size_t)L4_MsgWord(msg_p, 0);
	to_send = min(to_send,min((READBUF_SIZE+bufwrite-bufread)%READBUF_SIZE,READBUF_SIZE));

	// copy content of circular buffer to linear buffer
	char locbuf[READBUF_SIZE];
	memcpy(locbuf, &buf[bufread], READBUF_SIZE-bufread);
	memcpy(&locbuf[READBUF_SIZE-bufread], buf, bufread);

    // prepare IPC message
    L4_MsgClear(msg_p);

    // set string length
    L4_MsgAppendWord(msg_p, to_send);

    // add words to message
    char* locbufptr = locbuf;
    for (unsigned int i = 0; i < to_send/4+1 && i < IPC_MAX_WORDS; i++) {
    	L4_MsgAppendWord(msg_p, (*(L4_Word_t*)locbufptr) );
    	locbufptr += 4; // advanced 4 bytes forward
    }

    // make message ready to send
	L4_MsgLoad(msg_p);
}
