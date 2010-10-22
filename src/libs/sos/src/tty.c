#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "tty.h"

#include <l4/types.h>
#include <l4/kdebug.h>
#include <l4/ipc.h>

#include "../../../sos/syscalls.h"


#define MSG_WORD_SIZE 4
#define MSG_MAX_SIZE (MSG_WORD_SIZE*4) // we use 5 registers, 1 register is reserved for msg length
#define min(a,b) (((a) < (b)) ? (a) : (b))

static L4_ThreadId_t sSystemId;
static char write_buffer[MSG_MAX_SIZE];



void ttyout_init(void) {
	sSystemId = L4_Pager(); // Is this the correct way to do it?
}

size_t sos_write(const void *vData, long int position, size_t count, void *handle)
{
    //dprintf("Send message with: %s", (char*) vData);

    int not_sent_count = count;
  	char* realdata = (char*) vData;

    while(not_sent_count > 0) {

        int to_send = min(MSG_MAX_SIZE, not_sent_count);

        // fill up buffer
        memcpy(write_buffer, realdata, to_send);
        realdata += to_send;
        char* bufferp = write_buffer;

        // prepare IPC message
        L4_Msg_t msg;
        L4_MsgTag_t tag;
        L4_MsgClear(&msg);

        // set string length
	    L4_MsgAppendWord(&msg, to_send);

	    for(int offset = 0; offset < 4; offset += 1) {

	    	L4_MsgAppendWord(&msg, (*(L4_Word_t*)bufferp) );
	    	bufferp += 4; // advanced 4 bytes forward

	    }

        L4_Set_MsgLabel(&msg, SOS_SERIAL_WRITE << 4);
    	L4_MsgLoad(&msg);

		// TODO: check for error
    	tag = L4_Send(sSystemId);

    	if(L4_IpcFailed(tag))
    		return to_send;
        
        not_sent_count -= to_send;

    }

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
