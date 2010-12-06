/**
 * File System
 * =============
 * This file contains the handlers for read/write/open/close/stat/get_dirent
 * syscalls.
 *
 * All files are tracked in the `file_table`. The file table entries
 * are described io.h (`file_info`). The syscall handler here will usually
 * look up the related file on the file_table and call it's corresponding
 * open/close/read/write function. Which at the moment either point to
 * a corresponding function in io_serial.c or io_nfs.c depending on whether
 * we have to deal with a console or a nfs file.
 *
 * Open files are identified by a file descriptor which corresponds
 * to the index of the given file handle in the file table. The layout
 * of a `file_table_entry` is described in io.h.
 *
 * Every process comes with a pre initialized file descriptor 0 which
 * has write capability for the console file. Note that the
 * file descriptor 0 corresponds to the file descriptor stdout_fd.
 *
 * Limitations:
 * ------------------------------------
 * The NFS specific filehandles for every file as well as their attributes
 * are read from the filesytem in the beginning (through io_init) and stored
 * in a array called file_cache. This limits the files which we can handle
 * to DIR_CACHE_SIZE.
 *
 * We introduced a maximum path length for open calls (MAX_PATH_LENGTH)
 * atm this is 255 characters. Because of the way we share memory on IPC calls
 * a limit of PAGESIZE would be possible without much effort.
 *
 * The number of concurrently open files for a given process is
 * restricted by PROCESS_MAX_FILES (see process.h).
 */

#include <serial.h>
#include <string.h>
#include <assert.h>
#include <l4/ipc.h>
#include <l4/schedule.h>

#include "../mm/pager.h"
#include "../mm/swapper.h"
#include "../process.h"
#include "../libsos.h"
#include "../network.h"
#include "io.h"
#include "io_serial.h"
#include "io_nfs.h"

#define verbose 2

// Buffer for console read
#define READ_BUFFER_SIZE 0x1000
static char read_buffer[READ_BUFFER_SIZE];
static circular_buffer console_circular_buffer;

// Cache for directory entries
static int file_cache_next_entry = 0;
file_info* file_cache[DIR_CACHE_SIZE];

/**
 * Inserts a file_info struct into the file_cache.
 * @param fi pointer to the file_info to insert
 * @return The index were we inserted the file.
 */
inline int file_cache_insert(file_info* fi) {
	assert(file_cache_next_entry < DIR_CACHE_SIZE);

	fi->creation_pending = FALSE;
	file_cache[file_cache_next_entry] = fi;

	return file_cache_next_entry++;
}


/** Checks if a file descriptor is within the file table range and the entry is currently not NULL */
inline static L4_Bool_t is_valid_filedescriptor(L4_ThreadId_t tid, fildes_t fd) {
	file_table_entry** file_table = get_process(tid)->filetable;
	return 0 <= fd && fd < PROCESS_MAX_FILES && file_table[fd] != NULL;
}


/** Checks if a given thread can write to a file represented by file descriptor fd */
inline static L4_Bool_t can_write(L4_ThreadId_t tid, fildes_t fd) {
	file_table_entry** file_table = get_process(tid)->filetable;
	return is_valid_filedescriptor(tid, fd) && (file_table[fd]->mode & FM_WRITE);
}


/** Checks if a given thread can read to a file represented by file descriptor fd */
inline static L4_Bool_t can_read(L4_ThreadId_t tid, fildes_t fd) {
	file_table_entry** file_table = get_process(tid)->filetable;
	return is_valid_filedescriptor(tid, fd) && (file_table[fd]->mode & FM_READ);
}


/** Checks if a given thread can close a file represented by file descriptor fd */
inline static L4_Bool_t can_close(L4_ThreadId_t tid, fildes_t fd) {
	return is_valid_filedescriptor(tid, fd);
}


/**
 * Searches for a given file in the file cache.
 *
 * @param name of file to search for
 * @return index in file_cache or -1 if not found
 */
int find_file(data_ptr name) {

	for(int i=0; i<DIR_CACHE_SIZE; i++) {
		if(strcmp(file_cache[i]->filename, name) == 0)
			return i;
	}

	return -1;
}


/**
 * Finds a free file slot in a given file table.
 * @param file_table
 * @return Index to a free slot in the table or -1 if the
 * file table is full.
 */
fildes_t find_free_file_slot(file_table_entry** file_table) {

	for(int i=0; i<PROCESS_MAX_FILES; i++) {
		if(file_table[i] == NULL)
			return i;
	}

	return -1;
}


/**
 * Creates a file descriptor with given arguments.
 * @return file_table_entry struct
 */
file_table_entry* create_file_descriptor(file_info* fi, L4_ThreadId_t tid, fmode_t mode) {

	file_table_entry* fte = malloc(sizeof(file_table_entry)); // freed on close()
	assert(fte != NULL);

	fte->file = fi;
	fte->owner = tid;
	fte->to_read = 0;
	fte->to_write = 0;
	fte->read_position = 0;
	fte->write_position = 0;
	fte->client_buffer = NULL;
	fte->awaits_callback = FALSE;
	fte->mode = mode;

	return fte;
}


/**
 * Initializer is called in main.c by the sos server on startup (after network init).
 * This creates a special device called "console" and links it with the serial
 * console.
 * At the end we read out the NFS directory and fill up the file_cache with the
 * files found.
 */
void io_init() {

	// initializing console special file
	console_circular_buffer.read_position = 0;
	console_circular_buffer.buffer = (char*) &read_buffer;
	console_circular_buffer.write_position = 0;
	console_circular_buffer.overflow = FALSE;
	console_circular_buffer.size = READ_BUFFER_SIZE;

	file_info* console_file = malloc(sizeof(file_info)); // This is never free'd but its okay
	assert(console_file != NULL);
	strcpy(console_file->filename, "console");

	console_file->status.st_atime = 0;
	console_file->status.st_ctime = 0;
	console_file->status.st_fmode = FM_READ | FM_WRITE;
	console_file->status.st_size = 0;
	console_file->status.st_type = ST_SPECIAL;
	console_file->reader = L4_nilthread;
	console_file->open = &open_serial;
	console_file->read = &read_serial;
	console_file->write = &write_serial;
	console_file->close = &close_serial;
	console_file->serial_handle = console_init();
	console_file->cbuffer = &console_circular_buffer;

	file_cache_insert(console_file);

	// initialize swap file
	file_info* swap_file = create_nfs("swap", L4_nilthread, 0);
	// overwrite callbacks for swap file (because of different behaviour than standard io)

	// initialize file handle for swap file
	get_process(root_thread_g)->filetable[SWAP_FD] = create_file_descriptor(swap_file, root_thread_g, FM_READ | FM_WRITE);

	// Set up dir cache with files from NFS directory
	nfs_readdir(&mnt_point, 0, MAX_PATH_LENGTH, &nfs_readdir_callback, 0);
}


/**
 * System Call handler for open() calls. We make sure that we get a correct
 * IPC message and copy the filename to open in a location only accessible
 * by the root server.
 *
 * We return -1 to the callee if the IPC message has a wrong number of
 * arguments or the name is NULL or the mode is not a valid access mode.
 *
 * @param tid Caller Thread ID
 * @param msg_p IPC Message containing requested file access mode in Word 0
 * @param name filename requested for opening
 *
 */
int open_file(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {

	if(buf == NULL || L4_UntypedWords(msg_p->tag) != 1) {
		send_ipc_reply(tid, SOS_OPEN, 1, -1);
		return 0;
	}

	fmode_t mode = L4_MsgWord(msg_p, 0);
	if(mode != O_RDONLY && mode != O_RDWR && mode != O_WRONLY) {
		send_ipc_reply(tid, SOS_OPEN, 1, -1);
		return 0;
	}

	// make sure our name is really \0 terminated
	// so no client can trick us into reading garbage by calling string functions on this
	char name[MAX_PATH_LENGTH+1];
	memcpy(name, buf, MAX_PATH_LENGTH+1);
	name[MAX_PATH_LENGTH] = '\0';

	int index = -1;
	if( (index = find_file(name)) == -1 ) {
		// file does not exist, create file
		create_nfs(name, tid, mode);
	}
	else {
		file_info* fi = file_cache[index];
		fi->open(fi, tid, mode);
	}

	return 0; // callback handled by open/or create_nfs
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

	file_table_entry* f = get_process(tid)->filetable[fd];

	f->to_read = to_read;
	f->client_buffer = buf;

	f->file->read(f);

	return 0; // ipc return is done by dynamic read handler
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
	file_table_entry* f = get_process(tid)->filetable[fd];
	f->to_write = to_write;
	f->client_buffer = buf;
	f->file->write(f);

	return 0; // ipc return is done by dynamic read handler
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

	file_table_entry* f = get_process(tid)->filetable[fd];

	// for certain files we need to call a special close handler
	if(f->file->close != NULL) {
		f->file->close(f);
	}

	// free allocated structures
	free(f);
	get_process(tid)->filetable[fd] = NULL;

	return set_ipc_reply(msg_p, 1, 0);
}


/**
 * System call handler for the stat syscall.
 *
 * @param tid Caller Thread ID
 * @param msg_p IPC message
 * @param buf Contains the filename for the file we wan't to lookup
 * @return 1 because this system call can be handled directly.
 */
int stat_file(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {

	if(buf == NULL || L4_UntypedWords(msg_p->tag) != 0)
		return IPC_SET_ERROR(-1);

	char path[MAX_PATH_LENGTH+1];
	memcpy(path, buf, MAX_PATH_LENGTH+1);
	path[MAX_PATH_LENGTH] = '\0';

	int index = -1;
	if((index = find_file(path)) != -1) {
		memcpy(buf, &file_cache[index]->status, sizeof(stat_t));
		return set_ipc_reply(msg_p, 1, 0);
	}
	else {
		// file does not exist
		return IPC_SET_ERROR(-1);
	}
}


/**
 * System Call handler for getting the information about a directory entry.
 * This function does a lookup in the global IO file_cache.
 */
int get_dirent(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {

	if(buf == NULL || L4_UntypedWords(msg_p->tag) != 1)
		return IPC_SET_ERROR(-1);

	// which file position do we want to read
	int pos = L4_MsgWord(msg_p, 0);

	if(pos == file_cache_next_entry)
		return IPC_SET_ERROR(0);
	if(pos > file_cache_next_entry)
		return IPC_SET_ERROR(-1);

	int to_copy = strlen(file_cache[pos]->filename);
	strcpy(buf, file_cache[pos]->filename);
	return set_ipc_reply(msg_p, 1, to_copy);
}
