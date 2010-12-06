/* Simple operating system interface */

#ifndef _SOS_H
#define _SOS_H

#include <stdlib.h>
#include <l4/types.h>
#include <l4/ipc.h>
#include <tty.h>

#include <sos_shared.h>

/* The FD to which printf() will ultimately write() */
extern fildes_t stdout_fd;

L4_MsgTag_t system_call(int, L4_Msg_t*, int, ...);

/* I/O system calls */

/**
 * Open file and return file descriptor, -1 if unsuccessful
 * (too many open files, console already open for reading).
 * A new file should be created if 'path' does not already exist.
 * A failed attempt to open the console for reading (because it is already
 * open) will result in a context switch to reduce the cost of busy waiting
 * for the console.
 * "path" is file name, "mode" is one of O_RDONLY, O_WRONLY, O_RDWR.
 */
fildes_t open(const char *path, fmode_t mode);


/**
 * Closes an open file. Returns 0 if successful, -1 if not (invalid "file").
 */
int close(fildes_t file);


/**
 * Read from an open file, into "buf", max "nbyte" bytes.
 * Returns the number of bytes read.
 * Will block when reading from console and no input is presently
 * available. Returns -1 on error (invalid file, invalid buffer).
 */
int read(fildes_t file, char *buf, size_t nbyte);


/**
 * Write to an open file, from "buf", max "nbyte" bytes.
 * Returns the number of bytes written. <nbyte disk is full.
 * Returns -1 on error (invalid file, invalid buffer).
 */
int write(fildes_t file, const char *buf, size_t nbyte);


/**
 * Reads name of entry "pos" in directory into "name", max "nbyte" bytes.
 * Returns number of bytes returned, zero if "pos" is next free entry,
 * -1 if error (non-existent entry, invalid buffer).
 */
int getdirent(int pos, char *name, size_t nbyte);


/**
 * Returns information about file "path" through "buf".
 * Returns 0 if successful, -1 otherwise (invalid name, invalid buffer).
 */
int stat(const char *path, stat_t *buf);




/* Process system calls */

/**
 * Create a new process running the executable image "path".
 * Returns ID of new process, -1 if error (non-executable image, nonexisting
 * file).
 */
pid_t process_create(const char *path);


/**
 * This is an internal syscall used by the initializer after the ELF file has been loaded
 * into memory to tell the SOS server to start the binary. This syscall is only invoked
 * once (not by the user) after that it will no longer work and always return -1.
 */
int process_start(void);


/**
 * Delete process (and close all its file descriptors).
 * Returns 0 if successful, -1 otherwise (invalid process).
 */
int process_delete(pid_t pid);


/** Returns ID of caller's process. */
pid_t my_id(void);


/**
 * Returns through "processes" status of active processes (at most "max"),
 * returns number of process descriptors actually returned.
 */
int process_status(process_t *processes, unsigned max);


/**
 * Wait for process "pid" to exit. If "pid" is -1, wait for any process
 * to exit. Returns the pid of the process which exited.
 */
pid_t process_wait(pid_t pid);


/**
 * Returns time in microseconds since booting.
 */
//long time_stamp(void);
uint64_t time_stamp(void);

/**
 * Sleeps for the specified number of milliseconds.
 */
void sleep(int msec);




/* Optional (bonus) system calls*/

/**
 * Make VM region ["adr","adr"+"size") sharable by other processes.
 * If "writable" is non-zero, other processes may have write access to the
 * shared region. Both, "adr" and "size" must be divisible by the page size.
 *
 * In order for a page to be shared, all participating processes must execute
 * the system call specifying an interval including that page.
 * Once a page is shared, a process may write to it if and only if all
 * _other_ processes have set up the page as shared writable.
 *
 * Returns 0 if successful, -1 otherwise (invalid address or size).
 */
int share_vm(void *adr, size_t size, int writable);


/* Debug Syscalls */
void sos_debug_flush(void);

#endif
