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
#include <l4/schedule.h>

#include "io.h"
#include "io_serial.h"
#include "pager.h"
#include "process.h"
#include "libsos.h"
#include "network.h"

#define verbose 1

// Buffer for console read
#define READ_BUFFER_SIZE 0x1000
static char read_buffer[READ_BUFFER_SIZE];
static circular_buffer console_circular_buffer;

// Cache for directory entries
file_info* file_cache[DIR_CACHE_SIZE];

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
static int find_file(data_ptr name) {

	for(int i=0; i<DIR_CACHE_SIZE; i++) {
		if(strcmp(file_cache[i]->filename, name) == 0)
			return i;
	}

	return -1;
}

fildes_t find_free_file_slot(file_table_entry** file_table) {

	for(int i=0; i<PROCESS_MAX_FILES; i++) {
		if(file_table[i] == NULL)
			return i;
	}

	return -1;
}


/**
 * Initializer is called in main.c by the sos server on startup (after network init).
 * This creates a special device called "console" and links it with the serial
 * console.
 */
void io_init() {
	console_circular_buffer.read_position = 0;
	console_circular_buffer.buffer = (char*) &read_buffer;
	console_circular_buffer.write_position = 0;
	console_circular_buffer.overflow = FALSE;

	// initializing console special file
	file_cache[0] = malloc(sizeof(file_info)); // This is never free'd but its okay
	strcpy(file_cache[0]->filename, "console");
	file_cache[0]->size = 0;
	file_cache[0]->reader = L4_nilthread;
	file_cache[0]->open = &open_serial;
	file_cache[0]->read = &read_serial;
	file_cache[0]->write = &write_serial;
	file_cache[0]->close = &close_serial;
	file_cache[0]->serial_handle = console_init();
	file_cache[0]->cbuffer = &console_circular_buffer;

	// initialize standard out file descriptor
	file_table_entry** file_table = get_process(L4_nilthread)->filetable; // TODO make correct
	file_table[0] = malloc(sizeof(file_table_entry)); // freed on close()
	file_table[0]->file = file_cache[0];
	file_table[0]->owner = L4_nilthread;
	file_table[0]->to_read = 0;
	file_table[0]->destination = NULL;

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
int open_file(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {

	if(buf == NULL || L4_UntypedWords(msg_p->tag) != 1)
		return IPC_SET_ERROR(-1);

	fmode_t mode = L4_MsgWord(msg_p, 0);
	if(mode != O_RDONLY && mode != O_RDWR && mode != O_WRONLY)
		return IPC_SET_ERROR(-1);

	// make sure our name is really \0 terminated
	// so no client can trick us into reading garbage by calling string functions on this
	char name[MAX_PATH_LENGTH+1];
	memcpy(name, buf, MAX_PATH_LENGTH+1);
	name[MAX_PATH_LENGTH] = '\0';

	int index = -1;
	if( (index = find_file(name)) == -1 ) {
		// TODO: file does not exist, create file
		// index = create_file(name);
	}
	assert((index = find_file(name)) != -1); // file must exist now

	file_info* fi = file_cache[index];
	return fi->open(fi, tid, msg_p);
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
	f->destination = buf;

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
	int written = f->file->write(f, to_write, buf);

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



static void  stat_file_handler(uintptr_t token, int status, struct cookie * fh, fattr_t *attr) {
	dprintf(0, "stat_file_handler: got called with token:0x%X status:%d\n", token, status);
	L4_ThreadId_t recipient = (L4_ThreadId_t) token;

	if(status == NFS_OK) {
		stat_t* fst = pager_physical_lookup(recipient, (L4_Word_t)ipc_memory_start);
		fst->st_size = attr->size;
		fst->st_atime = attr->atime.useconds / 10;
		fst->st_ctime = attr->ctime.useconds;
		fst->st_type = ST_FILE;
		fst->st_fmode = 0; // TODO
		send_ipc_reply(recipient, SOS_STAT, 1, 0);
	}
	else {
		send_ipc_reply(recipient, SOS_STAT, 1, -1);
	}
}


int stat_file(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {

	if(buf == NULL || L4_UntypedWords(msg_p->tag) != 0)
		return IPC_SET_ERROR(-1);

	char path[MAX_PATH_LENGTH+1];
	memcpy(path, buf, MAX_PATH_LENGTH+1);
	path[MAX_PATH_LENGTH] = '\0';

	nfs_lookup(&mnt_point, path, &stat_file_handler, tid.raw);

	return 0; // handler returns to client
}


/**
 * NFS handler for getting the info about a directory entry.
 *
 * @param tid Caller thread ID
 * @param msg_p IPC Message
 * @param buf buffer where content is copied into
 */
static void get_dirent_cb(uintptr_t token, int status, int num_entries, struct nfs_filename *filenames, int next_cookie) {
	dprintf(0,"get_dirent_cb called!\n");
	dprintf(0,"number of file entries returned: %d\n", num_entries);
/*
	// copy file entries to local cache
	size_t entries = min(num_entries,DIR_CACHE_SIZE);
	for (int i = 0; i < entries; i++) {
		size_t to_copy = min(filenames[i].size,MAX_PATH_LENGTH-1);
		memcpy(dir_cache[i].filename,filenames[i].file,to_copy);
		dir_cache[i].filename[to_copy] = '\0';
	}
*/
	//L4_ThreadId_t recipient = (L4_ThreadId_t)token;
	//char* buf = (char*)pager_physical_lookup(recipient, (L4_Word_t)ipc_memory_start);

}

/**
 * Systam Call handler for getting the info about a directory entry.
 *
 * @param tid Caller thread ID
 * @param msg_p IPC Message
 * @param buf buffer where content is copied into
 */
int get_dirent(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {
	if(buf == NULL || L4_UntypedWords(msg_p->tag) != 1)
		return IPC_SET_ERROR(-1);

	// which file position do we want to read
	//int pos = L4_MsgWord(msg_p, 0);

	// call nfs_readdir
	nfs_readdir(&mnt_point,0,MAX_PATH_LENGTH,&get_dirent_cb,(uintptr_t)tid.raw);

	return 0;
}
