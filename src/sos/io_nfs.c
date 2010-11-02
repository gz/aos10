#include <assert.h>
#include <string.h>
#include <nfs.h>
#include "io_nfs.h"
#include "io.h"
#include "libsos.h"
#include "network.h"


#define verbose 1


static void nfs_set_status(uintptr_t token, int status, struct cookie* fh, fattr_t* attr) {

	int index = (int) token;
	stat_t* stat = &file_cache[index]->status;

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
				//fi->open = &nfs_open;
				//fi->read = &nfs_read;
				//fi->write = &nfs_write;
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

