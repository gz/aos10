/****************************************************************************
 *
 *      Description: Example startup file for the SOS project.
 *
 *      Author:		    Godfrey van der Linden(gvdl)
 *      Original Author:    Ben Leslie
 *
 ****************************************************************************/


#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

#include "syscalls.h"
#include "l4.h"
#include "libsos.h"
#include "network.h"
#include "sos_serial.h"

#include "pager.h"
#include "frames.h"

#define verbose 2

#define ONE_MEG	    (1 * 1024 * 1024)

#define HEAP_SIZE   ONE_MEG /* 1 MB heap */


/* Set aside some memory for a stack for the
 * first user task 
 */
#define STACK_SIZE 0x1000
static L4_Word_t init_stack_s[STACK_SIZE];
static L4_Word_t user_stack_s[STACK_SIZE];

// Init thread - This function starts up our device drivers and runs the first
// user program.
static void
init_thread(void)
{
    // Initialise the network for libsos_logf_init
    network_init();

    // Loop through the BootInfo starting executables
    int i;
    L4_Word_t task = 0;
    L4_BootRec_t *binfo_rec;
    for (i = 1; (binfo_rec = sos_get_binfo_rec(i)); i++) {
	if (L4_BootRec_Type(binfo_rec) != L4_BootInfo_SimpleExec)
	    continue;

	// Must be a SimpleExec boot info record
	dprintf(0, "Found exec: %d %s\n", i, L4_SimpleExec_Cmdline(binfo_rec));

	// Start a new task with this program
	L4_ThreadId_t newtid = sos_task_new(++task, L4_Pager(), 
		(void *) L4_SimpleExec_TextVstart(binfo_rec),
		(void *) &user_stack_s[STACK_SIZE]);
	dprintf(0, "Created task: %lx\n", sos_tid2task(newtid));
    }

    dprintf(0, "Calling frame test 1:\n");
    frame_test1();

    // Thread finished - block forever
    for (;;)
	sos_usleep(30 * 1000 * 1000);
}


#define TAG_SYSLAB(t)	((short) L4_Label(t) >> 4)

/*
  Syscall loop.

  This implements a very simple, single threaded functions for 
  recieving any IPCs and dispatching to the correct subsystem.

  It currently handles pagefaults, interrupts and special sigma0
  requests.
*/
static __inline__ void syscall_loop(void)
{
    dprintf(3, "Entering %s\n", __FUNCTION__);

    // Setup which messages we will recieve
    L4_Accept(L4_UntypedWordsAcceptor);
    
    int send = 0;
    L4_Msg_t msg;
    L4_ThreadId_t tid = L4_nilthread;

    for (;;) {
		L4_MsgTag_t tag;

		// Wait for a message, sometimes sending a reply
		if (!send)
			tag = L4_Wait(&tid); // Nothing to send, so we just wait
		else
			tag = L4_ReplyWait(tid, &tid); // Reply to caller and wait for IPC

		if (L4_IpcFailed(tag)) {
			L4_Word_t ec = L4_ErrorCode();
			dprintf(0, "%s: IPC error\n", __FUNCTION__);
			sos_print_error(ec);
			assert( !(ec & 1) );	// Check for recieve error and bail
			send = 0;
			continue;
		}

		// At this point we have, probably, recieved an IPC
		L4_MsgStore(tag, &msg); /* Get the tag */

		dprintf(2, "%s: got msg from %lx, (%d %p)\n", __FUNCTION__,
			 L4_ThreadNo(tid), (int) TAG_SYSLAB(tag),
			 (void *) L4_MsgWord(&msg, 0));

		//
		// Dispatch IPC according to protocol.
		//
		send = 1; /* In most cases we will want to send a reply */
		switch (TAG_SYSLAB(tag)) {
			case L4_PAGEFAULT:
				// A pagefault occured. Dispatch to the pager
				pager(tid, &msg);
			break;

			case L4_INTERRUPT:
				if (0); // Received an interrupt, you need to implement this side
				else
				network_irq(&tid, &send);
			break;

			/* our system calls */
			case SOS_SERIAL_WRITE:
				assert(L4_UntypedWords(tag) == 5); // make sure there were 5 registers filled with stuff
				sos_serial_send(&msg, &send);
			break;

			/* error? */
			default:
				// Unknown system call, so we don't want to reply to this thread
				sos_print_l4memory(&msg, L4_UntypedWords(tag) * sizeof(uint32_t));
				send = 0;
			break;
		}
    }
}

//
// Main entry point - called by crt.
//

int
main (void)
{
    // Initialise initial sos environment
    libsos_init();

    // Initialse the memory frame table

    // Find information about available memory
    L4_Word_t low, high;
    sos_find_memory(&low, &high);
    dprintf(0, "Available memory from 0x%08lx to 0x%08lx - %luMB\n", 
	   low, high, (high - low) / ONE_MEG);
    frame_init((low + HEAP_SIZE), high); // Initialise the frame table

    // Spawn the setup thread which completes the rest of the initialisation,
    // leaving this thread free to act as a pager and interrupt handler.
    (void) sos_thread_new(&init_thread, &init_stack_s[STACK_SIZE]);

    syscall_loop(); // Enter the syscall loop
    /* Not reached */

    return 0;
}
