#include <assert.h>
#include <l4/types.h>
#include <l4/ipc.h>

#include "libsos.h"
#include "process.h"
#include "io_serial.h"
#include "io.h"
#include "datastructures/circular_buffer.h"

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

static file_table_entry* get_reading_file_handle(file_info* fi) {
	file_table_entry** file_table = get_process(fi->reader)->filetable;

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


int open_serial(file_info* fi, L4_ThreadId_t tid, L4_Msg_t* msg_p) {
	assert(get_process(tid) != NULL);
	file_table_entry** file_table = get_process(tid)->filetable;
	fmode_t mode = L4_MsgWord(msg_p, 0);

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
				return IPC_SET_ERROR(-1);
			}

		}

		// if we come here we can create it's okay to create a file descriptor
		file_table[fd] = malloc(sizeof(file_table_entry)); // freed on close()
		assert(file_table[fd] != NULL);
		file_table[fd]->file = fi;
		file_table[fd]->owner = tid;
		file_table[fd]->to_read = 0;
		file_table[fd]->to_write = 0;
		file_table[fd]->client_buffer = NULL;
		file_table[fd]->mode = mode;

	}

	return set_ipc_reply(msg_p, 1, fd);
}


/**
 * Read function for special devices reading from a serial device.
 * This function is usually called in `read_file` or by a serial
 * interrupt. It only sends an IPC message back to the client
 * if we have something in the buffer. This realizes the blocking read
 * call on the client side.
 *
 * Note: Special Files using this function need at least a buffer of size
 * `READ_BUFFER_SIZE`.
 *
 * @param f callee
 */
void read_serial(file_table_entry* f) {

	L4_Word_t to_send = circular_buffer_read(f->file->cbuffer, f->to_read, f->client_buffer);

	if(to_send > 0) {
		send_ipc_reply(f->owner, CREATE_SYSCALL_NR(SOS_READ), 1, to_send);
		f->to_read = 0;
	}
}


/**
 * This is a write function for special files writing to a serial device.
 *
 * @param f special file entry (callee)
 * @param to_send amount of bytes to send
 * @param buffer send content
 * @return total bytes sent
 */
int write_serial(file_table_entry* f) {
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

	return total_sent;
}

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
