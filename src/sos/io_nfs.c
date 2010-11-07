/**
 * NFS I/O System
 * ==============
 * This files contains the handlers for read/write/open/getdirent/stat
 * system calls specific to the NFS filesystem.
 * The usual behaviour is that the create_nfs, read_nfs, and write_nfs
 * call the NFS library function which then calls a given callback function
 * in return. Usually we use the token value as a pointer to the file handle
 * and store all the information about what we need to do on a callback
 * in the file handle.
 *
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <nfs.h>
#include "io_nfs.h"
#include "io.h"
#include "libsos.h"
#include "network.h"
#include "process.h"

#define verbose 1


/**
 * Sets the status attributes for a given file_info struct.
 */
static void nfs_set_status_callback(uintptr_t token, int status, struct cookie* fh, fattr_t* attr) {

	int index = (int) token;
	stat_t* stat = &file_cache[index]->status;

	file_cache[index]->nfs_handle = *fh; // copy the file handle

	if(status == NFS_OK) {
		stat->st_size = attr->size;
		stat->st_atime = attr->atime.useconds / 1000;
		stat->st_ctime = attr->ctime.useconds / 1000;
		stat->st_type = ST_FILE;

		stat->st_fmode = 0; // reset because create_nfs stores information here
		if(attr->mode & 0000400)
			stat->st_fmode |= FM_READ;
		if(attr->mode & 0000200)
			stat->st_fmode |= FM_WRITE;
		if(attr->mode & 0000100)
			stat->st_fmode |= FM_EXEC;
	}
	else {
		dprintf(0, "%s: Bad status (%d) from callback.\n", __FUNCTION__, status);
	}

}

/**
 * Callback from NFS when a file has been created. Since files can only be
 * created through the open() system call we also create a file handle and return
 * it to the user.
 */
static void nfs_create_callback (uintptr_t token, int status, struct cookie* fh, fattr_t* attr) {
	file_info* fi = (file_info*) token;

	if(status == NFS_OK) {
		int inserted_at = file_cache_insert(fi); // TODO: should be after nfs_set_status
		int mode = fi->status.st_fmode;
		nfs_set_status_callback(inserted_at, NFS_OK, fh, attr);
		open_nfs(fi, fi->reader, mode);
	}
	else {
		dprintf(0, "%s: Bad status (%d) from callback.\n", __FUNCTION__, status);
		send_ipc_reply(fi->reader, SOS_OPEN, 1, -1);
	}

	fi->reader = L4_nilthread; // hack to know where requested this file
}


/**
 * Tells NFS to create a file in the given file system.
 *
 * @param name file name to create
 * @param recipient the user thread which called open()
 * @param the mode he wants to open this file in.
 */
void create_nfs(char* name, L4_ThreadId_t recipient, fmode_t mode) {

	file_info* fi = malloc(sizeof(file_info)); // This is never free'd but its okay
	assert(fi != NULL);
	strcpy(fi->filename, name);
	fi->cbuffer = NULL;
	fi->serial_handle = NULL;
	fi->open = &open_nfs;
	fi->read = &read_nfs;
	fi->write = &write_nfs;
	fi->close = NULL;

	fi->status.st_fmode = mode; // we abuse this field to know in which mode the client wants to open the file
	fi->reader = recipient; // we abuse this field to know where to send our reply in the callback

	sattr_t sat;
	sat.uid = 1000;
	sat.gid = 1000;
	sat.size = 0;
	sat.mode = 0000400 | 0000200; // set up for RW access

	nfs_create(&mnt_point, name, &sat, &nfs_create_callback, (int)fi);
}


/**
 * Opens a NFS file. Note that we don't need a callback here since
 * we already cached the NFS file handle on io_init for all NFS
 * files.
 *
 * @param fi pointer to the file we want to open
 * @param tid thread which wants to open
 * @param mode mode in which we want to open the file
 */
void open_nfs(file_info* fi, L4_ThreadId_t tid, fmode_t mode) {
	assert(get_process(tid) != NULL);
	file_table_entry** file_table = get_process(tid)->filetable;

	int fd = -1;
	if( (fd = find_free_file_slot(file_table)) != -1) {

		if((mode & fi->status.st_fmode) == mode) {
			// if we come here it's okay to create a file descriptor
			file_table[fd] = malloc(sizeof(file_table_entry)); // freed on close()
			assert(file_table[fd] != NULL);
			file_table[fd]->file = fi;
			file_table[fd]->owner = tid;
			file_table[fd]->to_read = 0;
			file_table[fd]->to_write = 0;
			file_table[fd]->read_position = 0;
			file_table[fd]->write_position = 0;
			file_table[fd]->client_buffer = NULL;
			file_table[fd]->mode = mode;

			send_ipc_reply(tid, SOS_OPEN, 1, fd);
			return;
		}
		else {
			dprintf(0, "thread:0x%X is trying to open file with mode:%d\nbut file access is restricted to %d\n", tid.raw, mode, fi->status.st_fmode);
		}

	}

	send_ipc_reply(tid, SOS_OPEN, 1, -1);
	return;
}


/**
 * NFS callback function we return the read bytes back to the user.
 * Note that a pointer to the file handle is passed as the token value.
 */
static void nfs_read_callback(uintptr_t token, int status, fattr_t *attr, int bytes_read, char *data) {

	file_table_entry* f = (file_table_entry*) token;

	if(status == NFS_OK) {
		f->read_position += bytes_read;
		memcpy(f->client_buffer, data, bytes_read);
		send_ipc_reply(f->owner, CREATE_SYSCALL_NR(SOS_READ), 1, bytes_read);
	}
	else {
		dprintf(0, "%s: Bad status (%d) from callback.\n", __FUNCTION__, status);
		send_ipc_reply(f->owner, CREATE_SYSCALL_NR(SOS_READ), 1, -1);
	}

}


/**
 * Tell NFS to read a certain amount of bytes at a given read position for
 * a given file.
 */
void read_nfs(file_table_entry* f) {
	nfs_read(&f->file->nfs_handle, f->read_position, f->to_read, &nfs_read_callback, (int)f);
}


/**
 * NFS callback function after we have written a given amount of bytes
 * into a file (file handle passed through token).
 * We also update the size and access time in the file_cache.
 */
static void nfs_write_callback(uintptr_t token, int status, fattr_t *attr) {

	file_table_entry* f = (file_table_entry*) token;

	if(status == NFS_OK) {
		int new_size = attr->size;
		f->write_position += f->to_write;

		// update file attributes
		f->file->status.st_size = new_size;
		f->file->status.st_atime = attr->atime.useconds / 1000;

		send_ipc_reply(f->owner, SOS_WRITE, 1, f->to_write);
	}
	else if(status == NFSERR_NOSPC) {
		send_ipc_reply(f->owner, SOS_WRITE, 1, 0); // no more space left on device
	}
	else {
		dprintf(0, "%s: Bad status (%d) from callback.\n", __FUNCTION__, status);
		send_ipc_reply(f->owner, SOS_WRITE, 1, -1);
	}

}


/**
 * Tell NFS to write to a given file.
 */
void write_nfs(file_table_entry* f) {
	nfs_write(&f->file->nfs_handle, f->write_position, f->to_write, f->client_buffer, &nfs_write_callback, (int)f);
}


/**
 * NFS handler for getting the info about a directory entry.
 * This handler will set up the file_cache cache of our root server.
 */
void nfs_readdir_callback(uintptr_t token, int status, int num_entries, struct nfs_filename *filenames, int next_cookie) {

	int entries = min(num_entries, DIR_CACHE_SIZE);
	if(entries > DIR_CACHE_SIZE) {
		dprintf(0, "Our directory cache cannot contain all entries, please increase DIR_CACHE_SIZE.\n");
	}

	// copy file entries to local cache
	for (int i = 0; i < entries; i++) {

		file_info* fi = malloc(sizeof(file_info)); // This is never free'd but its okay
		assert(fi != NULL);

		if(filenames[i].size < MAX_PATH_LENGTH-1) {

			int to_copy = min(filenames[i].size, MAX_PATH_LENGTH-1);
			memcpy(fi->filename, filenames[i].file, to_copy);
			fi->filename[to_copy] = '\0';

			if(strcmp(fi->filename, ".") > 0 && strcmp(fi->filename, "..") > 0) {

				fi->cbuffer = NULL;
				fi->serial_handle = NULL;
				fi->open = &open_nfs;
				fi->read = &read_nfs;
				fi->write = &write_nfs;
				fi->close = NULL;
				fi->reader = L4_anythread;

				int inserted_at = file_cache_insert(fi);
				nfs_lookup(&mnt_point, fi->filename, &nfs_set_status_callback, inserted_at);
			} // else ignore . and ..

		}
		else {
			dprintf(0, "Warning: Skipped files in NFS Directory with large filename. Increase MAX_PATH_LENGTH?\n");
		}
	}

	if(next_cookie > 0) {
		nfs_readdir(&mnt_point, next_cookie, MAX_PATH_LENGTH, &nfs_readdir_callback, 0);
	}

}

