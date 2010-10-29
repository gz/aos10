/**
 * Syscall Table
 * =============
 * The sysent table is used to transfer to the appropriate
 * routing for processing a system call. It's index corresponds
 * to the label sent with the IPC message from the client. So a
 * simple lookup in the table will give us the syscall handler
 * function (as it is done in the syscall loop).
 *
 * A syscall handler function has well defined arguments
 * 1. Thread ID of the sender
 * 2. The sent IPC Message
 * 3. A pointer to shared IPC memory between the root server and client
 *
 * As we can see, we have two ways to share content between the user and
 * the root server. First we can append up to 64 words to the IPC message.
 * And secondly we have the memory region starting in user space at
 * address 0x60000000 were we can insert a maximum 4096 bytes (pagesize)
 * of data. The root server in return will use the pager to look up the
 * physical address of this area and write it's response directly into
 * physical memory.
 * The convention we used was to store 4 byte words in IPC messages
 * and use the shared memory if we had strings or buffers to share.
 *
 * A syscall handler returns 0 or 1 which is used by the syscall loop
 * to determine if it should send back a message or not.
 *
 */

#include <assert.h>
#include <sos_shared.h>

#include "sysent.h"
#include "io.h"
#include "pager.h"

syscall_function_ptr sysent[SYSENT_SIZE] =  {
		[0 ... 15] = NULL
};

static void register_syscall(int ident, syscall_function_ptr func) {
	assert(sysent[ident] == NULL);

	sysent[ident] = func;
}

void init_systable() {
	register_syscall(SOS_OPEN, &open_file);
	register_syscall(SOS_READ, &read_file);
	register_syscall(SOS_WRITE, &write_file);
	register_syscall(SOS_CLOSE, &close_file);
	register_syscall(SOS_STAT, &stat_file);
	register_syscall(SOS_GETDIRENT, &get_dirent);

	register_syscall(SOS_UNMAP_ALL, &pager_unmap_all);
}
