/**
 * File System
 * =============
 * This file contains the handlers for read/write/open/close syscalls.
 *
 * Currently we support only special files (i.e. the console file).
 *
 * All files are tracked in the `file_table` and are identified by
 * a file descriptor which is corresponds to the index for the given
 * file in the file table. The layout of a `file_table_entry` is
 * described in io.h.
 * File descriptors with valued below SPECIAL_FILES are treated as
 * special files they can have multiple writers but only one reader
 * thread at a time.
 * The special file called console is created in io_init. It's
 * file descriptor is 0 which corresponds to the stdout_fd defined
 * in sos.h (libsos).
 *
 * Limitations:
 * ------------------------------------
 * To make the open call more secure we introduced a MAX_PATH_LENGTH
 * of 255 characters. Because of the way we share memory on IPC calls
 * a limit of PAGESIZE-1 would be
 *
 */

#include <serial.h>
#include <string.h>
#include <assert.h>
#include <l4/ipc.h>

#include "io.h"
#include "libsos.h"

#define verbose 1

#define IPC_SET_ERROR(err) set_ipc_reply(msg_p, 1, (err));

// Buffer for console read
#define READ_BUFFER_SIZE 0x1000
static char read_buffer[READ_BUFFER_SIZE];


/** Test to see whether a given file is a special file or not */
inline static L4_Bool_t is_special_file(file_table_entry* f) {
	return f != NULL && f->serial_handle != NULL;
}


/** Test to see whether a given file descriptor represents a special file or not */
inline static L4_Bool_t is_special_filedescriptor(fildes_t fd) {
	return fd >= 0 && fd < SPECIAL_FILES;
}


/** Checks if a file descriptor is within the file table range and the entry is currently not NULL */
inline static L4_Bool_t is_valid_filedescriptor(fildes_t fd) {
	return 0 <= fd && fd < FILE_TABLE_ENTRIES && file_table[fd] != NULL;
}


/** Checks if a given thread can write to a file represented by file descriptor fd */
inline static L4_Bool_t can_write(L4_ThreadId_t tid, fildes_t fd) {
	L4_Bool_t valid = is_valid_filedescriptor(fd);
	L4_Bool_t can_write = is_special_filedescriptor(fd) || L4_IsThreadEqual(file_table[fd]->owner, tid);
	return valid && can_write;
}


/** Checks if a given thread can read to a file represented by file descriptor fd */
inline static L4_Bool_t can_read(L4_ThreadId_t tid, fildes_t fd) {
	return is_valid_filedescriptor(fd) && L4_IsThreadEqual(file_table[fd]->owner, tid);
}


/** Checks if a given thread can close a file represented by file descriptor fd */
inline static L4_Bool_t can_close(L4_ThreadId_t tid, fildes_t fd) {
	return is_valid_filedescriptor(fd) && L4_IsThreadEqual(file_table[fd]->owner, tid);
}


/**
 * Searches for a given file in the special file table.
 *
 * @param name of file to search for
 * @return index in special_table or -1 if not found
 */
static fildes_t find_special_file(data_ptr name) {

	for(int i=0; i<SPECIAL_FILES; i++) {
		if(strcmp(file_table[i]->identifier, name) == 0)
			return i;
	}

	return -1;
}


/**
 * Finds a file handle entry with serial_handle pointing to the same
 * memory location as `ser`.
 *
 * @param ser struct to search for
 * @return Pointer to the corresponding file table entry or NULL if not found.
 */
static inline file_table_entry* get_file_handle_for_serial(struct serial* ser) {
	file_table_entry* f = NULL;

	for(int i=0; i<SPECIAL_FILES; i++) {
		if(file_table[i]->serial_handle == ser)
			f = file_table[i];
	}

	return f;
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
static void serial_receive_handler(struct serial* ser, char c) {

	// get file_table_entry pointer
	file_table_entry* f = get_file_handle_for_serial(ser);
	assert(f != NULL);

	if(f->double_overflow)
		return; // if the buffer is full for the 2nd time, drop whatever follows

	// add char to circular buffer
	f->buffer[f->write_position] = c;
	f->write_position = (f->write_position + 1) % READ_BUFFER_SIZE;

	// end of the buffer (but one char) has been reached, add newline and send text
	if ((f->write_position+2) % READ_BUFFER_SIZE == f->read_position) {

		f->buffer[f->write_position] = '\n';
		f->write_position = (f->write_position + 1) % READ_BUFFER_SIZE;

		if (!f->reader_blocking) {
			f->double_overflow = TRUE; // buffer is full for the 2nd time, drop whatever follows
			dprintf(0, "\nDouble console read buffer overflow. We're starting to loose things here :-(.\n");
			return;
		}

		f->read(f);
	}
	else if (c == '\n')
		f->read(f);
}


/**
 * Initializes serial struct and registers handler function
 * function to receive input data.
 */
static struct serial* console_init(void) {

	struct serial* ser = serial_init();
	assert(ser != NULL);

	serial_register_handler(ser, &serial_receive_handler);

	return ser;
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
static void read_serial(file_table_entry* f) {

	// check f->to_read against buffer fill and determine how many chars should be sent
	L4_Word_t max_send = min(f->to_read, READ_BUFFER_SIZE-1);
	L4_Word_t to_send = min((READ_BUFFER_SIZE + f->write_position - f->read_position) % READ_BUFFER_SIZE, max_send);

	if(to_send > 0 && f->reader_blocking) {

		dprintf(3, "copy_console_buffer: f->reader_blocking: %d\n", f->reader_blocking);

		// copy the contents of the circular buffer to the shared memory location
		if (f->read_position < f->write_position)
			memcpy(f->destination, &f->buffer[f->read_position], f->write_position - f->read_position);
		else {
			memcpy(f->destination, &f->buffer[f->read_position], READ_BUFFER_SIZE-f->read_position);
			memcpy(&f->destination[READ_BUFFER_SIZE - f->read_position], f->buffer, f->write_position);
		}

		// ensure that there is a newline at the end
		assert(f->destination[to_send-1] == '\n');

		L4_MsgTag_t tag;
		L4_Msg_t msg;

		L4_Set_MsgTag(tag);
	    L4_MsgClear(&msg);
	    L4_MsgAppendWord(&msg, to_send);

	    // Set Label and prepare message
	    L4_Set_MsgLabel(&msg, CREATE_SYSCALL_NR(SOS_READ));
	    L4_MsgLoad(&msg);

	    // Sending Message
		tag = L4_Reply(f->owner);

	    f->reader_blocking = FALSE;
	    f->double_overflow = FALSE;


		if(L4_IpcFailed(tag)) {
			L4_Word_t ec = L4_ErrorCode();
			dprintf(0, "%s: Console read IPC callback has failed. User thread not blocking?\n", __FUNCTION__);
			sos_print_error(ec);
			f->reader_blocking = TRUE;
		}
		else {
			// increment read pointer
			f->read_position = (f->read_position + to_send) % READ_BUFFER_SIZE;
		}
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


/**
 * Initializer is called in main.c by the sos server on startup (after network init).
 * This creates a special device called "console" and links it with the serial
 * console.
 */
void io_init() {

	file_table[0] = malloc(sizeof(file_table_entry)); // This is never free'd but its okay

	// initializing console special file
	strcpy(file_table[0]->identifier, "console");
	file_table[0]->owner = L4_nilthread;
	file_table[0]->reader_blocking = FALSE;
	file_table[0]->double_overflow = FALSE;
	file_table[0]->to_read = 0;
	file_table[0]->buffer = (data_ptr) &read_buffer;
	file_table[0]->write_position = 0;
	file_table[0]->read_position = 0;
	file_table[0]->destination = NULL;
	file_table[0]->serial_handle = console_init();
	file_table[0]->read = &read_serial;
	file_table[0]->write = &write_serial;

}


/**
 * System Call handler for open() calls. This functions works in the following
 * way:
 *  1. Check if there is a special (i.e. console) device with the given name
 * 	2. Check if there is a file in the file system with the given name [TODO]
 *  3. Create the file on the file system if not found yet [TODO]
 *
 * A IPC message is sent back to the callee containing the file descriptor
 * or -1 on error.
 *
 * TODO: A failed attempt to open the console for reading (because it is already
 * open) will result in a context switch to reduce the cost of busy waiting
 * for the console.
 *
 * @param tid Caller Thread ID
 * @param msg_p IPC Message containing requested file access mode in Word 0
 * @param name filename requested for opening
 *
 */
int open_file(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr name) {

	if(name == NULL || L4_UntypedWords(msg_p->tag) != 1)
		return IPC_SET_ERROR(-1);

	fmode_t mode = L4_MsgWord(msg_p, 0);
	fildes_t fd = -1;

	if(mode != O_RDONLY && mode != O_RDWR && mode != O_WRONLY)
		return IPC_SET_ERROR(-1);

	// make sure our name is really \0 terminated
	// so no client can trick us into reading garbage by calling string functions on this
	name[MAX_PATH_LENGTH+1] = '\0';

	if( (fd = find_special_file(name)) != -1 ) {
		file_table_entry* f = file_table[fd];

		// user tries to open a special file
		// special files allow multiple writer single reader
		if(mode == O_RDONLY || mode == O_RDWR) {

			// one process is allowed to open the special device for reading multiple times
			if(f->owner.raw == 0 || f->owner.raw == tid.raw)
				f->owner = tid;
			else
				return IPC_SET_ERROR(-1); // error: only one process allowed for writing

		}
		return set_ipc_reply(msg_p, 1, fd); // writing always allowed, return file descriptor

	} // else TODO: handle file system access


	// reply with either file descriptor or fd equals -1
	return set_ipc_reply(msg_p, 1, fd);
}


/**
 * Systam Call handler for reading a file.
 *
 * @param tid Caller thread ID
 * @param msg_p IPC Message
 * @param buf buffer where content is copied into
 */
int read_file(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {
	if(buf == NULL || L4_UntypedWords(msg_p->tag) != 2)
		return IPC_SET_ERROR(-1);

	int fd = L4_MsgWord(msg_p, 0);
	size_t to_read = L4_MsgWord(msg_p, 1);

	if(!can_read(tid, fd))
		return IPC_SET_ERROR(-1);

	file_table_entry* f = file_table[fd];

	if(is_special_file(f)) {

		f->reader_blocking = TRUE;
		f->to_read = to_read;
		f->destination = buf;

		f->read(f);
		return 0; // ipc return is handled by
				  // the serial interrupt (or f->read(f))
	}
	else {
		// TODO: file system read
		return IPC_SET_ERROR(-1);
	}
}


/**
 * System Call handler for writing to a file.
 * Calls the write function of the file entry and
 * returns bytes written to the callee through IPC.
 *
 * @param tid Caller thread ID
 * @param msg_p IPC message
 * @param buf buffer to write
 */
int write_file(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {

	if(buf == NULL || L4_UntypedWords(msg_p->tag) != 2)
		return IPC_SET_ERROR(-1);

	fildes_t fd = L4_MsgWord(msg_p, 0);
	int to_write = L4_MsgWord(msg_p, 1);

	if(!can_write(tid, fd) || to_write > IPC_MEMORY_SIZE)
		return IPC_SET_ERROR(-1);

	// do lookup and call write function
	file_table_entry* f = file_table[fd];
	int written = f->write(f, to_write, buf);

	return set_ipc_reply(msg_p, 1, written);
}


/**
 * System Call handler for closing a file.
 * For special files this function will unset the
 * reader_tid if the reader closes the file.
 *
 * @param tid Callee ID
 * @param msg_p IPC message
 * @param buf Shared memory pointer (ignored)
 */
int close_file(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {
	if(L4_UntypedWords(msg_p->tag) != 1)
		return IPC_SET_ERROR(-1);

	fildes_t fd = L4_MsgWord(msg_p, 0);

	if(!can_close(tid, fd))
		return IPC_SET_ERROR(-1);

	if(is_special_filedescriptor(fd)) {

		if(L4_IsThreadEqual(file_table[fd]->owner, tid)) {
			file_table[fd]->owner = L4_nilthread;
			file_table[fd]->reader_blocking = FALSE;
		}

		// closing special file is always a success
		// since we can have multiple writers which we don't track
		return set_ipc_reply(msg_p, 1, 0);

	}
	else {
		// TODO: close on file system
		return IPC_SET_ERROR(-1);
	}
}
