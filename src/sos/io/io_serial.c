#include <assert.h>
#include <l4/types.h>
#include <l4/ipc.h>

#include "../libsos.h"
#include "../process.h"
#include "../datastructures/circular_buffer.h"
#include "io.h"
#include "io_serial.h"

#define verbose 2


/**
 * Finds a file handle entry with serial_handle pointing to the same
 * memory location as `ser`.
 *
 * @param ser struct to search for
 * @return Pointer to the corresponding file table entry or NULL if not found.
 */
static inline file_info* get_file_for_serial(struct serial* ser) {
	file_info* f = NULL;

	for(int i=0; i<DIR_CACHE_SIZE; i++) {
		if(file_cache[i] != NULL && file_cache[i]->serial_handle == ser)
			f = file_cache[i];
	}

	return f;
}

/**
 * Searches the file table of the current reader process and returns
 * the handle which currently want's to read on the serial file.
 *
 * @param fi the serial file
 * @return the entry which at the moment wants to read on the serial file.
 */
static file_table_entry* get_reading_file_handle(file_info* fi) {
	file_table_entry** file_table = get_process(fi->reader)->filetable;
	assert(file_table != NULL);

	for(int i=0; i < PROCESS_MAX_FILES; i++) {
		if(file_table[i] != NULL && file_table[i]->file == fi && file_table[i]->to_read > 0)
			return file_table[i];
	}

	return NULL; // no reading file handle exists
}


/**
 * Initializes serial struct and registers handler function
 * function to receive input data.
 */
struct serial* console_init(void) {

	struct serial* ser = serial_init();
	assert(ser != NULL);

	serial_register_handler(ser, &serial_receive_handler);

	return ser;
}


/**
 * Interrupt handler for incoming chars on serial console.
 * This function will find the special file corresponding
 * to the serial struct. Then fill its buffer with
 * the incoming char and call it's write function.
 * The write function is responsible for determine if it's
 * time to send an IPC back to the client.
 *
 * @param serial structure identifying the serial console
 * @param c received char
 */
void serial_receive_handler(struct serial* ser, char c) {

	// get file_table_entry pointer
	file_info* fi = get_file_for_serial(ser);
	assert(fi != NULL);

	int written = circular_buffer_write(fi->cbuffer, 1, c);
	file_table_entry* fh = get_reading_file_handle(fi);

	// send if there is a reader and we're either at a newline or buffer has overflow
	if(fh != NULL && (c == '\n' || written == 0))
		fi->read(fh);

}


/**
 * Function called by the open_file syscall handler for serial
 * files.
 * The convention here is that we allow multiple writer process
 * but only one process can open a serial file (multiple times)
 * for reading.
 */
void open_serial(file_info* fi, L4_ThreadId_t tid, fmode_t mode) {
	assert(get_process(tid) != NULL);
	file_table_entry** file_table = get_process(tid)->filetable;

	int fd = -1;
	if( (fd = find_free_file_slot(file_table)) != -1) {

		// create file table entry
		if(mode == O_RDONLY || mode == O_RDWR) {

			// one process is allowed to open the special device for reading multiple times
			if(fi->reader.raw == 0 || fi->reader.raw == tid.raw) {
				fi->reader = tid;
			}
			else {
				// only one process allowed for writing
				L4_ThreadSwitch(fi->reader);
				send_ipc_reply(tid, SOS_OPEN, 1, -1);
				return;
			}

		}

		// if we come here we can create it's okay to create a file descriptor
		file_table[fd] = create_file_descriptor(fi, tid, mode);
	}

	send_ipc_reply(tid, SOS_OPEN, 1, fd);
	return;
}


/**
 * Read function for special devices reading from a serial device.
 * This function is usually called in `read_file` or by a serial
 * interrupt. It only sends an IPC message back to the client
 * if we have something in the buffer. This realizes the blocking read
 * call on the client side.
 *
 * @param f file table entry of the callee
 */
void read_serial(file_table_entry* f) {

	L4_Word_t to_send = circular_buffer_read(f->file->cbuffer, f->to_read, f->client_buffer);

	if(to_send > 0) {
		send_ipc_reply(f->owner, SOS_READ, 1, to_send);
		f->to_read = 0;
	}
}


/**
 * This is a write function for serial files. In case
 * the serial device buffer should overflow we retry 20 times.
 */
void write_serial(file_table_entry* f) {
	// serial struct must be initialized
	assert(f->file->serial_handle != NULL);

	int total_sent = 0;
	char* buffer = f->client_buffer;

	// sending to serial
	// In case we send faster than our serial driver allows we
	// retry until all data is sent or at most 20 times.
	for (int i=0; i < 20; i++) {

		int sent = serial_send(f->file->serial_handle, buffer, f->to_write-total_sent);
		buffer += sent;

		total_sent += sent;

		if(total_sent == f->to_write)
			break; // everything sent, can exit loop
		else
			dprintf(0, "sos_serial_send: serial driver's internal buffer fills faster than it can actually output data");
	}

	send_ipc_reply(f->owner, SOS_WRITE, 1, total_sent);
}


/**
 * Close handler for serial files. We need to make sure to unset
 * f->file->reader if this was the last file with read access
 * on this serial device for the closing process.
 */
void close_serial(file_table_entry* f) {

	if(f->mode & FM_READ) {

		// are we the last reader for this process reading on that serial interface?
		file_table_entry** file_table = get_process(f->owner)->filetable;
		L4_Bool_t last_reader = TRUE;

		for(int i=0; i < PROCESS_MAX_FILES; i++) {
			if(file_table[i] == NULL)
				continue;

			L4_Bool_t not_self = (f != file_table[i]);
			L4_Bool_t is_reader = file_table[i]->mode & FM_READ;
			L4_Bool_t same_serial = f->file->serial_handle == file_table[i]->file->serial_handle;
			if(not_self && is_reader && same_serial) {
				last_reader = FALSE;
				break;
			}

		}

		if(last_reader) {
			f->file->reader = L4_nilthread;
		}
		// else keep process as reader
	}

}
