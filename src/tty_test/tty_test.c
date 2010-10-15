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


static void sos_debug_flush() {

}

#define NPAGES 128

static void do_pt_test( char *buf )
{
    int i;
   // L4_Fpage_t fp;

    /* set */
    for(i = 0; i < NPAGES; i += 4)
    buf[i * 1024] = i;

    /* flush */
    //sos_debug_flush();

    /* check */
    for(i = 0; i < NPAGES; i += 4)
    assert(buf[i*1024] == i);
}

static void pt_test( void )
{
	if(1) {
    /* need a decent sized stack */
    char buf1[NPAGES * 1024];//, *buf2 = NULL;

    /* check the stack is above phys mem */
    assert((void *) buf1 > (void *) 0x2000000);

    /* stack test */
    do_pt_test(buf1);

    /* heap test
    buf2 = malloc(NPAGES * 1024);
    assert(buf2);

    //check the heap is above phys mem
    assert((void *) buf2 > (void *) 0x2000000);

    do_pt_test(buf2);
    free(buf2); */
	}
}



int main(void)
{

	pt_test();

	/* initialise communication */
    ttyout_init();


	L4_ThreadId_t myid;
    /*assert( ((int)&stack_space) > 0x2000000);
    stack_space[0] = 'a';
    stack_space[1025] = 'b';

    printf("stack addr: %X\n", (int)&stack_space);*/

    
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
