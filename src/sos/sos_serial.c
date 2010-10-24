#include <stddef.h>
#include <string.h>
#include <serial.h>
#include <assert.h>

#include <sos_shared.h>

#include "sos_serial.h"
#include "l4.h"
#include "libsos.h"

#define verbose 1

// struct used to communicate with the serial driver
struct serial* ser = NULL;

// buffer for incoming chars
#define READBUF_SIZE 0x1000
char buf[READBUF_SIZE];

unsigned int bufwrite = 0;
unsigned int bufread = 0;




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
