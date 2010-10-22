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
#define IPC_MAX_WORDS 64

static L4_ThreadId_t sSystemId;
static char write_buffer[MSG_MAX_SIZE];



void tty_init(void) {
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

        L4_Set_MsgLabel(&msg, TAG_SETSYSCALL(SOS_SERIAL_WRITE));
    	L4_MsgLoad(&msg);

		// send message to root server and wait for reply
    	tag = L4_Call(sSystemId);

    	// check if ipc failed
    	if(L4_IpcFailed(tag))
    		return to_send;
        
    	// get reply message, should be one word containing the number of written chars
    	assert(L4_UntypedWords(tag) == 1);
    	int sent = L4_MsgWord(&msg, 0);

    	// check if all chars have been written to sos_serial
    	if(to_send != sent)
    		return sent;

        not_sent_count -= to_send;

    }

	return count;
}


size_t sos_read(void *vData, long int position, size_t count, void *handle) {

    // prepare IPC message
    L4_Msg_t msg;
    L4_MsgTag_t tag;
    L4_MsgClear(&msg);

    // set count of chars to read
    L4_MsgAppendWord(&msg, (L4_Word_t)count);

    // construct message label and load message
    L4_Set_MsgLabel(&msg, TAG_SETSYSCALL(SOS_SERIAL_READ));
	L4_MsgLoad(&msg);

	// send message to root server and wait for reply
	tag = L4_Call(sSystemId);

	// check if ipc failed
	if(L4_IpcFailed(tag))
		return 0;

	// get reply message lengths (words and chars)
	size_t words = L4_UntypedWords(tag);
	assert(words >= 1);
	size_t received = (size_t)L4_MsgWord(&msg, 0);
	assert(received == 0 || words == (received-1)/4+1);

	// target buffer size
	size_t to_copy = min(received,count);

	// copy message contents to out buffer
	char* ptr = (char*)vData;
	unsigned int i = 1;
	while (to_copy > 4) {
		*ptr = L4_MsgWord(&msg, i);
		i++;
		to_copy -= 4;
	}
	// copy the last word in parts
	if (to_copy > 0) {
		L4_Word_t minibuf;
		minibuf = L4_MsgWord(&msg, i);
		memcpy(ptr,&minibuf,to_copy);
	}

	return min(received,count);

	//size_t i;
	//char *realdata = vData;
	//for (i = 0; i < count; i++) // Fix this to use your syscall
	//	realdata[i] = L4_KDB_ReadChar_Blocked();
	//return count;
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
