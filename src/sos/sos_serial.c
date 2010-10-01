#include <stddef.h>
#include <serial.h>

#include "sos_serial.h"
#include "l4.h"
#include "libsos.h"

#define verbose 1

// struct used to communicate with the serial driver
static struct serial* ser = NULL;

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
void sos_serial_send(L4_Msg_t* msg_p, int* send_p) {
	dprintf(1, "received SOS_SERIAL_WRITE\n");

	if(ser == NULL)
		ser = serial_init();

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
	do {
		sent = serial_send(ser, message_ptr, len-total_sent);
		message_ptr += sent;

		total_sent += sent;

		if(total_sent < len) {
			dprintf(0, "sos_serial_send: serial driver's internal buffer fills faster than it can actually output data");
		}

	// In case we send faster than our serial driver allows we
	// retry until all data is sent.
	} while( total_sent < len);

	*send_p = 0;
}
