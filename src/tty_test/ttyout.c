/****************************************************************************
 *
 *      $Id:  $
 *
 *      Description: Simple milestone 0 code.
 *      		     Libc will need sos_write & sos_read implemented.
 *
 *      Author:      Ben Leslie
 *
 ****************************************************************************/

#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>

#include "ttyout.h"

#include <l4/types.h>
#include <l4/kdebug.h>

static L4_ThreadId_t sSystemId;

void
ttyout_init(void) {
	/* Perform any initialisation you require here */
	sSystemId = L4_GlobalId(0, 0); // Fix this, get the rootserver's thread id
}

size_t
sos_write(const void *vData, long int position, size_t count, void *handle)
{
	size_t i;
	const char *realdata = vData;
	for (i = 0; i < count; i++) // Fix this to use your syscall
		L4_KDB_PrintChar(realdata[i]);
	return count;
}

size_t
sos_read(void *vData, long int position, size_t count, void *handle)
{
	size_t i;
	char *realdata = vData;
	for (i = 0; i < count; i++) // Fix this to use your syscall
		realdata[i] = L4_KDB_ReadChar_Blocked();
	return count;
}

void
abort(void)
{
	L4_KDB_Enter("sos abort()ed");
	while(1); /* We don't return after this */
}

void _Exit(int status) { abort(); }

