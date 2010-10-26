/**
 * This table switch used to transfer to the appropriate
 * routing for processing a system call.
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

	//register_syscall(SOS_UNMAP_ALL, &pager_unmap_all);
}
