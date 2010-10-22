#include <sos.h>

fildes_t stdout_fd = 0;

/* Open file and return file descriptor, -1 if unsuccessful
 * (too many open files, console already open for reading).
 * A new file should be created if 'path' does not already exist.
 * A failed attempt to open the console for reading (because it is already
 * open) will result in a context switch to reduce the cost of busy waiting
 * for the console.
 * "path" is file name, "mode" is one of O_RDONLY, O_WRONLY, O_RDWR.
 */
fildes_t open(const char *path, fmode_t mode) {
	return 0;
}

/* Closes an open file. Returns 0 if successful, -1 if not (invalid "file").
 */
int close(fildes_t file) {
	return 0;
}


/* Read from an open file, into "buf", max "nbyte" bytes.
 * Returns the number of bytes read.
 * Will block when reading from console and no input is presently
 * available. Returns -1 on error (invalid file, invalid buffer).
 */
int read(fildes_t file, char *buf, size_t nbyte) {
	return 0;
}


/* Write to an open file, from "buf", max "nbyte" bytes.
 * Returns the number of bytes written. <nbyte disk is full.
 * Returns -1 on error (invalid file, invalid buffer).
 */
int write(fildes_t file, const char *buf, size_t nbyte) {
	return -1;
}


/* Reads name of entry "pos" in directory into "name", max "nbyte" bytes.
 * Returns number of bytes returned, zero if "pos" is next free entry,
 * -1 if error (non-existent entry, invalid buffer).
 */
int getdirent(int pos, char *name, size_t nbyte) {
	return -1;
}

/* Returns information about file "path" through "buf".
 * Returns 0 if successful, -1 otherwise (invalid name, invalid buffer).
 */
int stat(const char *path, stat_t *buf) {
	return 0;
}
