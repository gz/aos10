#include <serial.h>
#include <string.h>
#include <assert.h>
#include <l4/ipc.h>

#include "io.h"
#include "libsos.h"
#include "sos_serial.h"


#define verbose 1

// Buffer for console read
#define READ_BUFFER_SIZE 0x10//00
static char read_buffer[READ_BUFFER_SIZE];

static int copy_console_buffer(file_table_entry* f) {

	// check f->to_read against buffer fill and determine how many chars should be sent
	L4_Word_t max_send = min(f->to_read,READ_BUFFER_SIZE);
	L4_Word_t to_send = min((READ_BUFFER_SIZE+f->pos_write-f->pos_read)%READ_BUFFER_SIZE,max_send);

    dprintf(0, "f->reader_blocking: %d\n", f->reader_blocking);

	if(to_send > 0 && f->reader_blocking) {

		// copy the contents of the circular buffer to the shared memory location
		if (f->pos_read < f->pos_write)
			memcpy(f->destination, &f->buffer[f->pos_read], f->pos_write - f->pos_read);
		else {
			memcpy(f->destination, &f->buffer[f->pos_read], READ_BUFFER_SIZE-f->pos_read);
			memcpy(&f->destination[READ_BUFFER_SIZE - f->pos_read], f->buffer, f->pos_write);
		}

		// ensure that there is a newline at the end
		assert(f->destination[to_send-1] == '\n');

		// debug output of destination buffer
		char* locbuf = malloc(to_send+1);
		memcpy(locbuf,f->destination,to_send);
		locbuf[to_send] = '\0';
	    dprintf(0, "f->destination: %s\n", locbuf);
	    free(locbuf);

		L4_MsgTag_t tag;
		L4_Msg_t msg;

		L4_Set_MsgTag(tag);
	    L4_MsgClear(&msg);
	    dprintf(0, "f->reader_tid: 0x%X\n", f->reader_tid.raw);
	    dprintf(0, "f->pos_read: %d\n", f->pos_read);
	    dprintf(0, "f->pos_write: %d\n", f->pos_write);
	    dprintf(0, "to_send: %d\n", to_send);

	    L4_MsgAppendWord(&msg, to_send);


	    // Set Label and prepare message
	    L4_Set_MsgLabel(&msg, SOS_READ << 4);
	    L4_MsgLoad(&msg);

	    // Sending Message
	    f->reader_blocking = FALSE;
		tag = L4_Reply(f->reader_tid);


		if(L4_IpcFailed(tag)) {
			L4_Word_t ec = L4_ErrorCode();
			dprintf(0, "%s: Console read IPC callback has failed. User thread not blocking?\n", __FUNCTION__);
			sos_print_error(ec);
			f->reader_blocking = TRUE;
		}
		else {
			f->pos_read = (f->pos_read + to_send) % READ_BUFFER_SIZE;
		    dprintf(0, "inc: f->pos_read: %d\n", f->pos_read);
		    dprintf(0, "inc: f->pos_write: %d\n", f->pos_write);
		}
	}

	return 0;

}


/**
 * Interrupt handler for incoming chars on serial console.
 *
 * @param serial structure identifying the serial console
 * @param c received char
 */
static void serial_receive_handler(struct serial* ser, char c) {
	// debug out
	dprintf(0,"%c",c);

	// get file_table_entry pointer
	file_table_entry* f = &special_table[0];
	assert(f != NULL);
	assert(f->reader_tid.raw != 0);

	// add char to circular buffer
	f->buffer[f->pos_write] = c;
	f->pos_write = (f->pos_write + 1) % READ_BUFFER_SIZE;

	// discard char if buffer is full
//	if (f->position == READ_BUFFER_SIZE) {
//		dprintf(0, "Console read buffer overflow. We're starting to loose things here :-(.\n");
//		return;
//	}

	// if end of the buffer (but one char) has been reached, add newline and send text
	if ((f->pos_write+2) % READ_BUFFER_SIZE == f->pos_read) {
		dprintf(0, "Console read buffer overflow. We're starting to loose things here :-(.\n");
	    dprintf(0, "f->pos_read: %d\n", f->pos_read);
	    dprintf(0, "f->pos_write: %d\n", f->pos_write);
		f->buffer[f->pos_write] = '\n';
		f->pos_write = (f->pos_write + 1) % READ_BUFFER_SIZE;
	    dprintf(0, "f->pos_read: %d\n", f->pos_read);
	    dprintf(0, "f->pos_write: %d\n", f->pos_write);
		copy_console_buffer(f);
	}
	// if newline found, just send text
	else if (c == '\n')
		copy_console_buffer(f);
}

/**
 * Initializes serial struct and registers handler function
 * function to receive input data.
 *
 */
static struct serial* console_init(void) {

	struct serial* ser = serial_init();
	assert(ser != NULL);

	serial_register_handler(ser, &serial_receive_handler);

	return ser;
}


/**
 *
 */
static int write_serial(file_table_entry* f, int to_send, char* buffer) {
	// serial struct must be initialized
	assert(f->serial_handle != NULL);

	int total_sent = 0;

	// sending to serial
	// In case we send faster than our serial driver allows we
	// retry until all data is sent or at most 20 times.
	for (int i=0; i < 20; i++) {

		int sent = serial_send(f->serial_handle, buffer, to_send-total_sent);
		buffer += sent;

		total_sent += sent;

		if(total_sent == to_send)
			break; // everything sent, can exit loop
		else
			dprintf(0, "sos_serial_send: serial driver's internal buffer fills faster than it can actually output data");
	}

	return total_sent;
}


inline static L4_Bool_t is_special_file(file_table_entry* f) {
	return f != NULL && f->serial_handle != NULL;
}


void io_init() {
	dprintf(0, "io_init called\n");
	// initializing console special file
	strcpy(special_table[0].identifier, "console");
	special_table[0].reader_tid = L4_nilthread;
	special_table[0].reader_blocking = FALSE;
	special_table[0].to_read = 0;
	special_table[0].buffer = (data_ptr) &read_buffer;
	special_table[0].pos_write = 0;
	special_table[0].pos_read = 0;
	special_table[0].destination = NULL;
	special_table[0].serial_handle = console_init();
	//special_table[0].serial_handle->file = &special_table[0];
	special_table[0].read = NULL;
	special_table[0].write = &write_serial;

	//assert(special_table[0].serial_handle->file == &special_table[0]);
}


void open_file(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {
	dprintf(0, "got open file..\n");
}


int read_file(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {
	int fd = L4_MsgWord(msg_p, 0);
	int to_read = L4_MsgWord(msg_p, 1);

	//dprintf(0, "to_read is: %d\n", to_read);
	//dprintf(0, "fd is: %d\n", fd);

	assert(fd == 0); // TODO: only console supported right now

	file_table_entry* f = &special_table[fd];

	if(is_special_file(f)) {

		f->reader_tid = tid; // TODO: hack
		f->reader_blocking = TRUE;
		f->to_read = to_read;
		f->destination = buf;

		copy_console_buffer(f);
	}


    return 0;
}


/**
 *
 * @param tid
 * @param msg_p
 * @param buf
 */
void write_file(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {
	int fd = L4_MsgWord(msg_p, 0);
	assert(fd == 0); // TODO: only console supported right now

	file_table_entry* f = &special_table[fd];

	int to_write = L4_MsgWord(msg_p, 1);
	int written = f->write(f, to_write, buf);

	L4_MsgClear(msg_p);
    L4_MsgAppendWord(msg_p, written);
    L4_MsgLoad(msg_p);
}
