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
#include <sos_shared.h>
#include <clock.h>
#include <nslu2.h>

#include "l4.h"
#include "libsos.h"
#include "network.h"

#include "mm/pager.h"
#include "mm/frames.h"
#include "io/io.h"
#include "process.h"
#include "sysent.h"

#include "../libs/c/src/k_r_malloc.h"

#define verbose 2

#define ONE_MEG	    (1 * 1024 * 1024)
#define HEAP_SIZE   ONE_MEG*4 /* 4 MB heap */

/** Set aside some memory for a init stack */
#define STACK_SIZE 0x1000
static L4_Word_t init_stack_s[STACK_SIZE];


/** Init thread - This function starts up our device drivers and runs the first user program. */
static void init_thread(void)
{
    // Initialise the network for libsos_logf_init
    start_timer();
	network_init();
	io_init();

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
		L4_ThreadId_t newtid = sos_task_new(++task, L4_Pager(), (void *) L4_SimpleExec_TextVstart(binfo_rec), (void *) 0xC0000000);
		create_process(newtid);

		dprintf(0, "Created task: %lx\n", sos_tid2task(newtid));
    }

    // Thread finished - block forever
    for (;;)
	sos_usleep(30 * 1000 * 1000);
}


/**
 * Syscall loop
 * This implements a simple, single threaded server for
 * receiving IPCs and dispatching to the correct subsystem.
 *
 * Note: This function should never return.
 */
static __inline__ void syscall_loop(void)
{
    dprintf(3, "Entering %s\n", __FUNCTION__);

    init_systable();

    L4_Accept(L4_UntypedWordsAcceptor);
    
    int reply = 0;
    L4_Msg_t msg;
    L4_ThreadId_t tid = L4_nilthread;

    for (;;) {

		L4_MsgTag_t tag;

		// Wait for a message, sometimes sending a reply
		if (!reply)
			// Nothing to send, so we just wait
			tag = L4_Wait(&tid);
		else
			// Reply to caller and wait for IPC
			tag = L4_ReplyWait(tid, &tid);

		if (L4_IpcFailed(tag)) {
			L4_Word_t ec = L4_ErrorCode();
			dprintf(0, "%s: IPC error\n", __FUNCTION__);
			sos_print_error(ec);
			assert( !(ec & 1) ); // Check for receive error and bail
			reply = 0;
			continue;
		}

		// At this point we have, probably, received an IPC
		L4_MsgStore(tag, &msg);

		dprintf(2, "%s: got msg from %lx (version:%lx), (%d %p)\n", __FUNCTION__, L4_ThreadNo(tid), L4_Version(tid), (int) GET_SYSCALL_NR(tag), (void *) L4_MsgWord(&msg, 0));

		reply = 1; // In most cases we will want to send a reply
		int sysnr = GET_SYSCALL_NR(tag);

		// Dispatch IPC according to protocol.
		switch (sysnr) {

			// Handle L4 Pagefaults
			case L4_PAGEFAULT:
				pager(tid, &msg);
			break;

			// Handle Interrupts
			case L4_INTERRUPT:
				if(L4_ThreadNo(tid) == NSLU2_TIMESTAMP_IRQ) // Received an interrupt, you need to implement this side
					reply = timer_overflow_irq(tid, &msg);
				else if(L4_ThreadNo(tid) == NSLU2_TIMER0_IRQ)
					reply = timer0_irq(tid, &msg);
				else
					network_irq(&tid, &reply);
			break;

			// Handle System calls
			case 0 ... SYSENT_SIZE-1:
			{
				data_ptr ipc_memory = pager_physical_lookup(tid, (L4_Word_t)ipc_memory_start);

				if(sysent[sysnr] != NULL) {
					reply = sysent[sysnr](tid, &msg, ipc_memory);
				}
				else {
					dprintf(0, "Syscall#%d not supported by SOS Server.\n", sysnr);
					set_ipc_reply(&msg, 1, -1);
				}
			}
			break;

			// Handle unknown IPC messages
			default:
				dprintf(0, "Got unknown system call (%d).\n", sysnr);
				sos_print_l4memory(&msg, L4_UntypedWords(tag) * sizeof(L4_Word_t));
				reply = 0;
			break;
		}

    }
}


/** Main entry point - called by crt. */
int main(void)
{
    // Initialize initial sos environment
	libsos_init();

    // Find information about available memory
    L4_Word_t low, high;
    sos_find_memory(&low, &high);

	__malloc_init((void*) low, (void*) ((low + HEAP_SIZE)-1));

    dprintf(0, "Available memory from 0x%08lx to 0x%08lx - %luMB\n", low, high, (high - low) / ONE_MEG);

    // Initialize memory management
    frame_init((low + HEAP_SIZE), high);
    pager_init();

    // Initialize process structure and register root process
    process_init();
    create_process(L4_Myself());

    // Spawn the setup thread which completes the rest of the initialization,
    // leaving this thread free to act as a pager and interrupt handler.
    sos_thread_new(&init_thread, &init_stack_s[STACK_SIZE]);

    syscall_loop();

    /* Not reached */
    return 0;
}
