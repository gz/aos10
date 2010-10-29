#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "sos.h"
#include "tty.h"

#include <l4/types.h>


size_t sos_write(const void *vData, long int position, size_t count, void *handle) {
	return write(stdout_fd, vData, count);
}


size_t sos_read(void *vData, long int position, size_t count, void *handle) {
	return read(stdout_fd, vData, count);
}

