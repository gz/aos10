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
#include <string.h>

#include "ttyout.h"

#include <l4/types.h>
#include <l4/kdebug.h>
#include <l4/ipc.h>



#define MSG_WORD_SIZE 4
#define MSG_MAX_SIZE (MSG_WORD_SIZE*4) // we use 5 registers, 1 register is reserved for msg length
#define min(a,b) (((a) < (b)) ? (a) : (b))

static L4_ThreadId_t sSystemId;



void ttyout_init(void) {
	sSystemId = L4_Pager(); // Is this the correct way to do it?
}

size_t sos_write(const void *vData, long int position, size_t count, void *handle)
{
    //dprintf("Send message with: %s", (char*) vData);
	L4_Word_t* bufferp = (L4_Word_t*) vData;

	// prepare IPC message
	L4_Msg_t msg;
	L4_MsgTag_t tag;
	L4_MsgClear(&msg);

	// set string length
	L4_MsgAppendWord(&msg, count); // 1st register length of the string
	L4_MsgAppendWord(&msg, (L4_Word_t) bufferp); // 2nd register pointer to the memory location of the string

	// sending message
	L4_Set_MsgLabel(&msg, 1<<4);
	L4_MsgLoad(&msg);
	tag = L4_Send(sSystemId);

	return count;
}


size_t sos_read(void *vData, long int position, size_t count, void *handle) {
	size_t i;
	char *realdata = vData;
	for (i = 0; i < count; i++) // Fix this to use your syscall
		realdata[i] = L4_KDB_ReadChar_Blocked();
	return count;
}

void abort(void) {
	L4_KDB_Enter("sos abort()ed");
	while(1); /* We don't return after this */
}

void _Exit(int status) { abort(); }


/*size_t dprint(const void *vData, size_t count)
{
        size_t i;
        const char *realdata = vData;
        for (i = 0; i < count; i++)
                L4_KDB_PrintChar(realdata[i]);
        return count;
}*/
