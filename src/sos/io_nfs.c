#include <assert.h>
#include <string.h>
#include <nfs.h>
#include "io_nfs.h"
#include "io.h"
#include "libsos.h"
#include "network.h"
#include "process.h"

#define verbose 1

static void nfs_set_status(uintptr_t token, int status, struct cookie* fh, fattr_t* attr) {

	int index = (int) token;
	stat_t* stat = &file_cache[index]->status;

	file_cache[index]->nfs_handle = *fh; // copy the file handle

	if(status == NFS_OK) {
		stat->st_size = attr->size;
		stat->st_atime = attr->atime.useconds / 1000;
		stat->st_ctime = attr->ctime.useconds / 1000;
		stat->st_type = ST_FILE;

		if(attr->mode & 0000100)
			stat->st_fmode = FM_EXEC;
		else
			stat->st_fmode = FM_READ | FM_WRITE;
	}
	else {
		dprintf(0, "%s: Bad status (%d) from callback.\n", __FUNCTION__, status);
	}

}


int open_nfs(file_info* fi, L4_ThreadId_t tid, L4_Msg_t* msg_p) {
	assert(get_process(tid) != NULL);
	file_table_entry** file_table = get_process(tid)->filetable;
	fmode_t mode = L4_MsgWord(msg_p, 0);

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

			return set_ipc_reply(msg_p, 1, fd);
		}
		else {
			dprintf(0, "thread:0x%X is trying to open file with mode:%d\nbut file access is restricted to %d\n", tid.raw, mode, fi->status.st_fmode);
			return IPC_SET_ERROR(-1);
		}

	}

	return IPC_SET_ERROR(-1);
}


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


void read_nfs(file_table_entry* f) {
	nfs_read(&f->file->nfs_handle, f->read_position, f->to_read, &nfs_read_callback, (int)f);
}


static void nfs_write_callback(uintptr_t token, int status, fattr_t *attr) {

	file_table_entry* f = (file_table_entry*) token;

	if(status == NFS_OK) {
		int new_size = attr->size;
		int bytes_written = new_size - f->file->status.st_size;
		f->write_position += bytes_written;

		// update file attributes
		f->file->status.st_size = new_size;
		f->file->status.st_atime = attr->atime.useconds / 1000;

		send_ipc_reply(f->owner, CREATE_SYSCALL_NR(SOS_WRITE), 1, bytes_written);
	}
	else {
		dprintf(0, "%s: Bad status (%d) from callback.\n", __FUNCTION__, status);
		send_ipc_reply(f->owner, CREATE_SYSCALL_NR(SOS_WRITE), 1, -1);
	}

}

void write_nfs(file_table_entry* f) {
	nfs_write(&f->file->nfs_handle, f->write_position, f->to_write, f->client_buffer, &nfs_write_callback, (int)f);
}


/**
 * NFS handler for getting the info about a directory entry.
 * This handler will set up the directory cache of our root server.
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
				nfs_lookup(&mnt_point, fi->filename, &nfs_set_status, inserted_at);
			} // else ignore . and ..

		}
		else {
			dprintf(0, "Warning: Skipped files in NFS Directory with large filename. Increase MAX_PATH_LENGTH?\n");
		}
	}

}

