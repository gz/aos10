#include <serial.h>
#include <string.h>
#include <assert.h>
#include <l4/ipc.h>

#include "io.h"
#include "libsos.h"
#include "sos_serial.h"


#define verbose 1

// Buffer for console read
#define READ_BUFFER_SIZE 0x1000
static char read_buffer[READ_BUFFER_SIZE];
static unsigned int read_buffer_count = 0;


static int copy_console_buffer(file_table_entry* f) {
	dprintf(0, "copy_console_buffer for: 0x%X\n", f->reader_tid);

	if(f->position > 0 && f->reader_blocking) {
		memcpy(f->destination, f->buffer, f->position);

		L4_Msg_t msg;
	    L4_MsgClear(&msg);

    	L4_MsgAppendWord(&msg, f->position);

	    // Set Label and prepare message
	    L4_Set_MsgLabel(&msg, SOS_READ << 4);
	    L4_MsgLoad(&msg);

	    // Sending Message
	    dprintf(0, "sending msg to: 0x%X\n", f->reader_tid);
		L4_MsgTag_t tag = L4_Send_Nonblocking(f->reader_tid);

		if(L4_IpcFailed(tag)) {
			dprintf(0, "Console read IPC callback has failed. User thread not blocking?\n");
		}
		else {
			f->reader_blocking = FALSE;
			f->position = 0;
		}
	}

	dprintf(0, "end: copy_console_buffer for: 0x%X\n", f->reader_tid);

	return 0;

}


/**
 * Interrupt handler for incoming chars on serial console.
 *
 * @param serial structure identifying the serial console
 * @param c received char
 */
static void serial_receive(struct serial* ser, char c) {
	assert(read_buffer_count <= READ_BUFFER_SIZE);

	file_table_entry* f = &special_table[0];

	assert(f != NULL);
	assert(f->reader_tid.raw != 0);

	// discard char if buffer is full
	if(f->position == READ_BUFFER_SIZE) {
		dprintf(0, "Console read buffer overflow. We're starting to loose things here :-(.\n");
		return;
	}

	dprintf(0,"%c",c);

	f->buffer[f->position++] = c;

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

	serial_register_handler(ser, &serial_receive);

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
	special_table[0].buffer = (data_ptr) &read_buffer;
	special_table[0].position = 0;
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
	dprintf(0, "got read syscall from: %d\n", tid);

	int fd = L4_MsgWord(msg_p, 0);
	assert(fd == 0); // TODO: only console supported right now

	file_table_entry* f = &special_table[fd];

	if(is_special_file(f)) {
		dprintf(0, "special file handling\n");

		f->reader_tid = tid; // TODO: hack
		f->reader_blocking = TRUE;
		f->destination = buf;

		copy_console_buffer(f);
	}

	//int to_read = L4_MsgWord(msg_p, 1);

	//int read = sos_serial_send(to_read, buf);

	/*L4_MsgClear(msg_p);
    L4_MsgAppendWord(msg_p, sent);
    L4_MsgLoad(msg_p);*/
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

	file_table_entry f = special_table[fd];

	int to_write = L4_MsgWord(msg_p, 1);
	int written = f.write(&f, to_write, buf);

	L4_MsgClear(msg_p);
    L4_MsgAppendWord(msg_p, written);
    L4_MsgLoad(msg_p);
}
