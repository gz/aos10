#include "sos.h"

#include <assert.h>
#include <stdarg.h>
#include <l4/kdebug.h>
#include <l4/ipc.h>

L4_MsgTag_t system_call(int type, L4_Msg_t* msg_p, int args, ...) {
	assert(args < IPC_MAX_WORDS);

    L4_Accept(L4_UntypedWordsAcceptor);
    L4_MsgClear(msg_p);

    // Appending Data Words
	va_list ap;
	va_start(ap, args);
    for(int i = 0; i < args; i++) {
    	L4_MsgAppendWord(msg_p, va_arg(ap, L4_Word_t));
    }
    va_end(ap);

    // Set Label and prepare message
    L4_Set_MsgLabel(msg_p, CREATE_SYSCALL_NR(type));
    L4_MsgLoad(msg_p);

    // Sending Message
	L4_MsgTag_t tag = L4_Call(L4_Pager());
	if(L4_IpcFailed(tag)) {
		printf("System Call# %d has failed.", type);
	}

	L4_MsgStore(tag, msg_p);

	return tag;
}


void abort(void) {
	L4_KDB_Enter("sos abort()ed");
	while(1); /* We don't return after this */
}


void _Exit(int status) { abort(); }
