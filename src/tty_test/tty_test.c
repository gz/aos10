/****************************************************************************
 *
 *      $Id:  $
 *
 *      Description: Simple milestone 0 test.
 *
 *      Author:			Godfrey van der Linden
 *      Original Author:	Ben Leslie
 *
 ****************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <l4/types.h>

#include <l4/ipc.h>
#include <l4/message.h>
#include <l4/thread.h>

#include "ttyout.h"

// Block a thread forever
static void
thread_block(void)
{
    L4_Msg_t msg;

    L4_MsgClear(&msg);
    L4_MsgTag_t tag = L4_Receive(L4_Myself());

    if (L4_IpcFailed(tag)) {
	printf("blocking thread failed: %lx\n", tag.raw);
	*(char *) 0 = 0;
    }
}

int main(void)
{
    L4_ThreadId_t myid;
    char stack_space[1024*1024];
    assert( ((int)&stack_space) > 0x2000000);

    /* initialise communication */
    ttyout_init();
    
    myid = L4_Myself();
    do {
		printf("task:\tHello world, I'm\t0x%lx!\n", L4_ThreadNo(myid));

		sos_write("123456789012345\n", 0, 16, NULL);
		sos_write("1234567890123456789\n", 0, 20, NULL);
		sos_write("abcdefghijklmnop\n", 0, 17, NULL);
		sos_write("abc\n", 0, 4, NULL);

		thread_block();
		// sleep(1);	// Implement this as a syscall
    } while(1);
    
    return 0;
}
