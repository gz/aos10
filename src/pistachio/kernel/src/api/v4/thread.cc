/*********************************************************************
 *                
 * Copyright (C) 2002-2004,  Karlsruhe University
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *                
 * File path:     api/v4/thread.cc
 * Description:   thread manipulation
 *                
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *                
 * $Id: thread.cc,v 1.88 2005/01/18 12:51:55 cvansch Exp $
 *                
 ********************************************************************/

#include <l4.h>
#include INC_API(config.h)
#include INC_API(tcb.h)
#include INC_API(thread.h)
#include INC_API(interrupt.h)
#include INC_API(schedule.h)
#include INC_API(space.h)
#include INC_API(kernelinterface.h)
#include INC_GLUE(syscalls.h)
#include INC_API(syscalls.h)
#include INC_API(smp.h)

#include <kdb/tracepoints.h>

DECLARE_TRACEPOINT(SYSCALL_THREAD_CONTROL);

//#define TRACE_XCPU(x...) printf(x)
#define TRACE_XCPU(x...)

#ifdef CONFIG_DEBUG
static void strcpy(char * dest, const char * src)
{
    while(*src)
	*dest++ = *src++;
    *dest = '\0';
}
#endif

void handle_ipc_error (void);

bool tcb_t::is_interrupt_thread ()
{
    return (get_global_id().is_interrupt() &&
	    get_global_id().get_threadno() < get_kip()->thread_info.get_system_base());
}

/**
 * Detect a receive-redirector loop, using tortoise & hare algorithm
 */
bool redirector_loop_detect(threadid_t dest, threadid_t redirector)
{
    tcb_t * t = NULL;
    bool move_t = false;
    threadid_t redir_tid = redirector;
    threadid_t t_tid = redirector;

    if (redirector.is_nilthread())
	return false;
    if (dest == redirector)
	return true;

    while ( !redir_tid.is_anythread() )
    {
	tcb_t * redir = get_current_space()->get_tcb(redir_tid);
	/* If thread does not exist, this is ok */
	if (redir->get_global_id() != redir_tid)
	    return false;

	if (move_t)
	{
	    t = get_current_space()->get_tcb(t_tid);
	    t_tid = t->get_recv_redirector();
	}

	if (redir == t)
	    return true;

	redir_tid = redir->get_recv_redirector();
	move_t = !move_t;
    }

    return false;
}

/**
 * Stub invoked after a startup message has been received from the
 * thread's pager.
 */
static void thread_startup()
{
    tcb_t * current = get_current_tcb();

#if 0
    printf ("Startup %t: ip=%p  sp=%p\n", current,
	    current->get_mr (1), current->get_mr (2));
#endif

    // Poke received IP/SP into exception frame (or whatever is used
    // by the architecture).  No need to check for valid IP/SP.
    // Thread will simply fault if values are not valid.
    current->set_user_ip((addr_t)current->get_mr(1));
    current->set_user_sp((addr_t)current->get_mr(2));
    current->set_saved_state (thread_state_t::aborted);
	thread_state_lock();
    current->set_state (thread_state_t::running);
	thread_state_unlock();
}


/**
 * Fake that thread is waiting for IPC startup message.
 *
 * @param tcb		thread to wait for startup message
 * @param pager		thread id to receive startup message from
 */
static void fake_wait_for_startup (tcb_t * tcb, threadid_t pager)
{
    // Fake that we are waiting to receive untyped words from our
    // pager.
	thread_state_lock();
    tcb->set_state (thread_state_t::waiting_forever);
    tcb->set_partner (pager);
	thread_state_unlock();
    tcb->set_acceptor (acceptor_t::untyped_words());

    // Make sure that unwind will work on waiting thread.
    tcb->set_saved_partner (threadid_t::nilthread ());

    /* #warning VU: revise fake_wait_for_startup */
    // Make sure that IPC abort will restore user-level exception
    // frame instead of trying to return from IPC syscall.
    tcb->set_saved_state (thread_state_t::running);
}


void thread_return()
{
    /* normal return function - do nothing */
}

bool tcb_t::init(threadid_t dest)
{
    ASSERT(DEBUG, this);
    if (!allocate())
	return false;

    /* clear utcb and space */
    utcb = NULL;
    space = NULL;

    /* make sure, nobody messes around with the thread */
	thread_state_lock();
    set_state(thread_state_t::aborted);
    set_saved_state (get_state());
    partner = threadid_t::nilthread();
    set_saved_partner (threadid_t::nilthread());
	thread_state_unlock();

    set_send_redirector (threadid_t::anythread());
    set_recv_redirector (threadid_t::anythread());

    /* set thread id */
    myself_global = dest;
    utcb_location = ANYLOCALTHREAD.get_raw();

    /* initialize thread resources */
    resources.init(this);

    /* queue initialization */
    queue_state.init();
    send_head = NULL;

#if defined(CONFIG_MDOMAINS) || defined(CONFIG_MUNITS)
    
    /* #warning VU: uninitialized threads assigned to CPU */
    /* initially assign to this CPU */
    set_context(get_current_context().root_context());
#endif

    /* initialize scheduling */
    get_current_scheduler()->init_tcb(this);

    /* enqueue into present list, do not enqueue
     * the idle thread since they are CPU local and break
     * the pointer list */
    if (this != get_idle_tcb())
	enqueue_present();

    init_stack();
    return true;
}


void tcb_t::create_kernel_thread(threadid_t dest, utcb_t * utcb)
{
    //TRACEF("dest=%t\n", TID(dest));
    init(dest);
    this->utcb = utcb;

    // kernel threads have prio 0 by default
    get_current_scheduler()->set_priority(this, 0);

    get_kernel_space()->add_tcb(this);
    /* Cannot do this - Bad interaction with ARM fastpath on irq
     * reply messages
     */
    //this->set_space(get_kernel_space());
}

bool tcb_t::create_inactive(threadid_t dest, threadid_t scheduler)
{
    //TRACEF("tcb=%t, %t\n", this, TID(dest));
    if (!init(dest))
	return false;

    this->scheduler = scheduler;
    return true;
}

bool tcb_t::activate(void (*startup_func)(), threadid_t pager)
{
    ASSERT(DEBUG, this);
    ASSERT(NORMAL, this->space);
    ASSERT(NORMAL, !this->is_activated());

    // UTCB location has already been checked during thread creation.
    // No need to check it again.  Just do an assert.

    //TRACEF("%x (%x)\n", get_utcb_location(), ((1 << get_kip()->utcb_info.alignment) - 1));

    /* allocate UTCB */
    this->utcb = get_space()->allocate_utcb(this);
    if (!this->utcb)
	return false;
    //ASSERT(DEBUG, check_utcb_location());

    /* update global id in UTCB */
    set_global_id (this->get_global_id());

    /* initialize scheduler, pager and exception handler */
    set_pager(pager);
    set_scheduler(this->scheduler);
    set_exception_handler(NILTHREAD);

    /* initialize the startup stack */
    create_startup_stack(startup_func);
    return true;
}

/**
 * Check if supplied address is a valid UTCB location.  It is assumed
 * that the space of the thread is properly initialized.
 *
 * @param utcb_address		location of UTCB
 *
 * @return true if UCTB location is valid, false otherwise
 */
bool tcb_t::check_utcb_location (word_t utcb_address)
{
#if (KIP_UTCB_SIZE != 0)
    return (get_kip()->utcb_info.is_valid_utcb_location(utcb_address) &&
		    get_space ()->get_utcb_page_area ().is_range_in_fpage
		    ((addr_t) utcb_address, (addr_t) (utcb_address + sizeof (utcb_t))));
#else
    /* UTCB is kernel allocated, utcb_address must be 0 */
    return utcb_address == 0;
#endif
}

/**
 * Check UTCB location of thread is valid.  It is assumed that the
 * space of the thread is properly initialized.
 *
 * @return true if UCTB location is valid, false otherwise
 */
bool tcb_t::check_utcb_location (void)
{
    return check_utcb_location (get_utcb_location ());
}


#ifdef CONFIG_MDOMAINS
/* All of this stuff needs to change */
#if 0
static void do_xcpu_delete_done(cpu_mb_entry_t * entry)
{
    tcb_t * tcb = entry->tcb;
    ASSERT(NORMAL, tcb);

    /* still on same CPU? --> otherwise forward */
    if (!tcb->is_local_cpu())
	UNIMPLEMENTED();

	thread_state_lock();
    if (tcb->get_state() != thread_state_t::xcpu_waiting_deltcb) {
		thread_state_unlock();
		UNIMPLEMENTED();
	}

    tcb->set_state(thread_state_t::running);
    tcb->xcpu_status = entry->param[0];
    thread_state_unlock();

    get_current_scheduler()->enqueue_ready(tcb);
}

static void xcpu_delete(tcb_t *tcb, tcb_t *src)
{
    /* Thread is already dequeued by caller */

    if (tcb->is_local_domain())
    {
	ASSERT(ALWAYS, get_current_tcb() != tcb);
	get_current_scheduler()->scheduler_lock();
	thread_state_lock();
	tcb->delete_tcb_prelocked();
	thread_state_unlock();
	get_current_scheduler()->scheduler_unlock();
    }


    xcpu_request(src->get_context(), do_xcpu_delete_done, src, done);
}

static void do_xcpu_delete(cpu_mb_entry_t * entry)
{
    tcb_t * tcb = entry->tcb;
    tcb_t * src = (tcb_t*)entry->param[0];
    tcb_t * current = get_current_tcb();
    word_t done = 1;

    //TRACEF("xcpu_delete of %t requested by %t\n", tcb, src);

    get_current_scheduler()->scheduler_lock();
    thread_state_lock();

    if (get_current_scheduler()->dequeue_thread_if_local_prelocked(tcb)) {

	/* prevent thread from being scheduled */
	tcb->context_bitmask = 0;

	if (current == tcb) {
	    thread_state_unlock();
	    get_current_scheduler()->scheduler_unlock();

	    ASSERT(ALWAYS, tcb != get_idle_tcb());

	    get_idle_tcb()->notify(xcpu_delete, tcb, src);
	    current->switch_to_idle();
	    return;
	}

	tcb->delete_tcb_prelocked();

	thread_state_unlock();
	get_current_scheduler()->scheduler_unlock();

	done = 0;
    } else {
	    thread_state_unlock();
	    get_current_scheduler()->scheduler_unlock();
    }

    xcpu_request(src->get_context(), do_xcpu_delete_done, src, done);
}
#endif
#endif
#ifdef CONFIG_MUNITS
static void do_xcpu_reschedule(cpu_mb_entry_t * entry)
{
    if(!get_current_scheduler()->schedule (get_current_tcb()) &&
	    (get_current_tcb()->context_bitmask & (1 << 
	    get_current_context().unit))==0)
	if (get_current_tcb() != get_idle_tcb())
	    get_current_tcb()->switch_to_idle();
    
}
#endif


word_t tcb_t::pause()
{
#ifdef CONFIG_MUNITS
    /* prevent thread from being scheduled */
    word_t bitmask = this->context_bitmask;

pause_retry:
    this->context_bitmask = 0;

    /* Stop the thread if it is currently running somewhere */
    if(this->is_scheduled())
    {
	cpu_context_t context = this->get_context();
	
	/* Make REALLY sure guy is not running on this hw_thread */
	ASSERT(ALWAYS, this != get_current_tcb());
	SMT_ASSERT(ALWAYS, context != get_current_context());

	/* IPI hw_thread other guy is running on */
	if(context != context.root_context())
	    xcpu_request(context, do_xcpu_reschedule);

	/* Spin and wait for him to stop */
	while(this->is_scheduled());
    }

    thread_state_lock();
    /* We do not want the thread to be locked waiting */
    if(this->get_state().is_locked_waiting() || this->get_state().is_locked_running())
    {
	thread_state_unlock();
	/* ok to check this outside of lock as we take lock and recheck soon
	 * Ie this is only a hint
	 * Don't bother checking locked running here as time between locked running
	 * being set and set_scheduled is very small 
	 */
	while(this->get_state().is_locked_waiting());
	thread_state_lock();
	if(this->get_state().is_locked_waiting() || this->get_state().is_locked_running() || this->is_scheduled())
	{
	    thread_state_unlock();
	    goto pause_retry;
	}
    }
    /* This should not be possible */
    ASSERT(ALWAYS, !this->get_state().is_locked_running());
    ASSERT(ALWAYS, !this->get_state().is_locked_waiting());
    thread_state_unlock();

    return bitmask;
#else /* CONFIG_MUNITS */
    return 1;
#endif
}

/* I think we need a pre-locked delete-tcb so no one can sneak in while we are
 * deleting
 */

void tcb_t::delete_tcb()
{
    ASSERT(ALWAYS, this->exists());
    
    this->pause();

    get_current_scheduler()->scheduler_lock();
    thread_state_lock();

    this->delete_tcb_prelocked();

    thread_state_unlock();
    get_current_scheduler()->scheduler_unlock();
}

void tcb_t::delete_tcb_prelocked() 
{
    if ( is_activated() )
    {
	ASSERT(NORMAL, get_utcb());
	SMT_ASSERT(ALWAYS, this->is_scheduled() == false);

	// dequeue from ready queue
	get_current_scheduler()->dequeue_ready_prelocked(this);

	ASSERT(ALWAYS, this->queue_state.is_set(queue_state_t::ready) == false);

	
	// unwind ongoing IPCs
	if (get_state().is_sending() || get_state().is_receiving()) {
	    unwind (tcb_t::abort);
	}
	
	// free any used resources
	resources.free(this);

	// free utcb
	this->utcb = (utcb_t*)NULL;
	this->set_state(thread_state_t::aborted);

	// cancel pending send requests
	while(send_head)
	{
	    // SMT: this is safe to do where
	    tcb_t * tcb = send_head;

	    ASSERT(ALWAYS, tcb != this);

	    thread_state_unlock();
	    get_current_scheduler()->scheduler_unlock();
	    tcb->pause();
	    get_current_scheduler()->scheduler_lock();
	    thread_state_lock();

	    SMT_ASSERT(ALWAYS, tcb->is_scheduled() == false);

	    tcb->unwind(tcb_t::abort);

	    tcb->notify(handle_ipc_error);
	    get_current_scheduler ()->enqueue_ready_prelocked (tcb);
	}
    } 

    // clear ids
    this->myself_global = NILTHREAD;
    this->utcb_location = ANYLOCALTHREAD.get_raw();
    this->set_state(thread_state_t::aborted);

    this->set_space(NULL);
    dequeue_present();

#ifdef CONFIG_DEBUG
    debug_name[0] = '\0';
#endif
}


/**
 * Caclulate the sender and receiver errorcodes when an IPC operation
 * has been aborted either by exchange_registers.
 *
 * @param reason		reason for abort
 * @param snd			sender thread
 * @param rcv			receiver thread
 * @param err_s			returned sender error code
 * @param err_r			returned receiver error code
 */
static void calculate_errorcodes (tcb_t::unwind_reason_e reason,
				  tcb_t * snd, tcb_t * rcv,
				  word_t * err_s, word_t * err_r)
{
    *err_s = IPC_SND_ERROR (ERR_IPC_ABORTED);
    *err_r = IPC_RCV_ERROR (ERR_IPC_ABORTED);
}


#if defined(CONFIG_MDOMAINS) || defined (CONFIG_MUNITS)

DECLARE_TRACEPOINT (XCPU_UNWIND);

/**
 * Handler invoked when our IPC partner has aborted/timed out the IPC.
 * Arguments are as follows:
 *
 * param[0]	thread id of partner
 * param[1]	expected thread state
 * param[2]	message tag to return
 * param[3]	error code to return
 */
static void do_xcpu_unwind_partner (cpu_mb_entry_t * entry)
{
    tcb_t * tcb = entry->tcb;
    threadid_t partner_id = threadid (entry->param[0]);
    thread_state_t expected_state (entry->param[1]);
    msg_tag_t tag = msgtag (entry->param[2]);
    word_t err = entry->param[3];

    TRACEPOINT (XCPU_UNWIND,
		printf ("tcb=%t, partner=%t, state=%s, tag=%p, err=%p\n",
			TID (tcb->get_global_id ()), TID (partner_id),
			expected_state.string (), tag.raw, err));

    if (EXPECT_FALSE (! tcb->is_local_unit ()))
    {
	// Forward request.
	xcpu_request (tcb->get_context(), do_xcpu_unwind_partner, tcb,
		      entry->param[0], entry->param[1],
		      entry->param[2], entry->param[3]);
	return;
    }

    thread_state_lock();
    if (EXPECT_FALSE (tcb->get_state () != expected_state) ||
	EXPECT_FALSE (tcb->get_partner () != partner_id))
    {
	thread_state_unlock();
	// Request is outdated.
	return;
    }

    if (! tcb->get_saved_partner ().is_nilthread ())
    {
	// We have a nested IPC operation.  Perform another unwind.
	tcb->restore_state ();
	tcb->unwind (tcb_t::abort);
    }
    else
    {
	tcb->set_error_code (err);
	tcb->set_tag (tag);
	tcb->set_state (thread_state_t::running);
    }
    thread_state_unlock();

    // Reactivate thread.
    tcb->notify (handle_ipc_error);
    get_current_scheduler ()->enqueue_ready_prelocked (tcb);
}

#endif /* CONFIG_MDOMAINS */


DECLARE_TRACEPOINT (UNWIND);

/**
 * Unwinds a thread from an ongoing IPC.
 * Assumes thread state lock is held by caller
 * @param reason		reason for unwind (abort or timeout)
 */
void tcb_t::unwind (unwind_reason_e reason)
{
    msg_tag_t tag = get_tag ();
    thread_state_t cstate;
    tcb_t * partner;

    TRACEPOINT (UNWIND,
		printf ("Unwind: tcb=%t p=%t s=%s (saved: p=%t s=%s)\n",
			TID (get_global_id ()), TID (get_partner ()),
			get_state ().string (),	TID (get_saved_partner ()),
			get_saved_state ().string ()));

	
    thread_state_t orig_cstate UNUSED = get_state ();
    thread_state_t orig_sstate UNUSED = get_saved_state ();
    threadid_t orig_cpartner = get_partner ();
    threadid_t orig_spartner = get_saved_partner ();

redo_unwind:

    cstate = get_state ();
    set_state (thread_state_t::running);
    partner = get_partner_tcb ();

    if (cstate.is_polling () || cstate.is_waiting () || cstate.is_waiting_notify ())
    {
	// IPC operation has not yet started.  I.e., partner is not
	// yet involved.

	if (cstate.is_polling ())
	{
	    // The thread is enqueued in the send queue of the partner.
	    dequeue_send (partner);
	}
	else
	{
	    // Thread is not yet receiving.  Cancel receive phase.
	    tag = tag.error_tag ();
	}

	// Set appropriate error code
	tag.set_error ();
	set_tag (tag);
	word_t err = ERR_IPC_CANCELED;
	set_error_code ((cstate.is_polling ()) ?
		       IPC_SND_ERROR (err) : IPC_RCV_ERROR (err));
	return;
    }

    else if (cstate == thread_state_t::locked_running)
    {
	// Thread is an active sender.  Abort both threads.

	word_t err_s, err_r;
	calculate_errorcodes (reason, this, partner, &err_s, &err_r);
	tag.set_error ();

#if defined(CONFIG_MDOMAINS)
	/* this is ok because it is an asynch request */
	if (! partner->is_local_domain ())
	{
	    printf("unwind: UNTESTED locked_running calling xcpu do_xcpu_unwind_partner\n");
	    thread_state_unlock();
	    xcpu_request (partner->get_context (), do_xcpu_unwind_partner,
			  partner, get_global_id ().get_raw (),
			  (word_t) thread_state_t::locked_waiting,
			  tag.raw, err_r);
	    thread_state_lock();
	}
	else
#endif
	{
	    // The receiver should be locked waiting.  It might also
	    // happen that we just completed a pagefault RPC without
	    // being scheduled yet.  In this case, our partner will
	    // not be communicating with us (and we should not abort
	    // any of his ongoing IPC operations).

	    if (partner->get_state () == thread_state_t::locked_waiting &&
		partner->get_partner () == get_global_id ())
	    {
		partner->set_error_code (err_r);
		partner->set_tag (tag);
		partner->set_state (thread_state_t::running);
		partner->notify (handle_ipc_error);
		/* FIXME: EVIL cannot do this */
		ASSERT(ALWAYS, !"Unsafe unwind");
		get_current_scheduler ()->enqueue_ready_prelocked (partner);
	    }
	}

	if (! get_saved_partner ().is_nilthread ())
	{
	    restore_state ();
	    goto redo_unwind;
	}

	set_tag (tag);
	set_error_code (err_s);
	return;
    }

    else if (cstate.is_xcpu_waiting ())
    {
	// Waiting for xcpu TCB deletion or exregs completion.  Just
	// ignore.
	return;
    }

    else if (cstate.is_running ())
    {
	// May happen, e.g., when doing a redo_unwind on a synthesized
	// IPC (e.g., pagefault) that was aborted.
	return;
    }

    WARNING ("Unresolved unwind: tcb=%t p=%t s=%s (saved: p=%t s=%s)\n",
	     TID (get_global_id ()),
	     TID (orig_cpartner), orig_cstate.string (),
	     TID (orig_spartner), orig_sstate.string ());
    return;
}


bool tcb_t::migrate_to_space(space_t * space)
{
    ASSERT(NORMAL, space);
    ASSERT(DEBUG, get_space());
    space_t *old_space = get_space();

    // allocate utcb in destination space
    utcb_t * new_utcb = space->allocate_utcb(this);
    if (!new_utcb)
	return false;

    this->pause();

    // make sure nobody messes around with the tcb
    if (this->is_activated()) {
	get_current_scheduler()->scheduler_lock();
	thread_state_lock();
	unwind (tcb_t::abort);
	thread_state_unlock();
	get_current_scheduler()->scheduler_unlock();
    }

    // remove from old space
    if (old_space->remove_tcb(this))
    {
	old_space->free();
	free_space(old_space);
    }

    // now change the space and set new UTCB ptr
    this->set_space(space);
    this->utcb = new_utcb;
    space->add_tcb(this);

    return true;
}

#ifdef CONFIG_MDOMAINS

/**
 * releases all resources held by the thread
 */
static void mp_release_thread(tcb_t * tcb)
{

    // thread is on the current CPU
    get_current_scheduler()->dequeue_ready(tcb);

    // remove from wakeup queue, we use the absolute timeout
    // as identifier whether a thread was in the wakeup list or not
    if (!tcb->queue_state.is_set(queue_state_t::wakeup))
		tcb->absolute_timeout = 0;
    else
		get_current_scheduler()->cancel_timeout(tcb);

    tcb->resources.purge(tcb);

    // when migrating IRQ threads disable interrupts
    if (tcb->is_interrupt_thread())
	migrate_interrupt_start (tcb);
}

/**
 * re-integrates a thread into the CPUs queues etc.
 */
static void xcpu_integrate_thread(tcb_t * tcb)
{
    ASSERT(NORMAL, !tcb->queue_state.is_set(queue_state_t::wakeup));

    // interrupt threads are handled specially
    if (tcb->is_interrupt_thread())
    {
	migrate_interrupt_end(tcb);
	return;
    }

    /* VU: the thread may have received an IPC meanwhile hence we
     * check whether the thread is already running again.  to make it
     * fully working the waiting timeout must be set more carefull! */
	thread_state_lock();
    if (tcb->get_state().is_runnable()) {
	thread_state_unlock();
	get_current_scheduler()->enqueue_ready(tcb);
   } else {
   	thread_state_unlock();
   }

}

/**
 * puts the thread onto the CPU the function is invoked on
 */
static void do_xcpu_put_thread(cpu_mb_entry_t * entry)
{
    tcb_t * tcb = entry->tcb;
    TRACE_XCPU("CPU%d (%t): %s %t\n", get_current_cpu(), 
	       get_current_tcb(), __func__, tcb);

    xcpu_integrate_thread(tcb);
}

void xcpu_put_thread(tcb_t * tcb, word_t tmp)
{
    cpu_context_t context;
    context.raw = tmp;

    TRACE_XCPU("%t migrating thread %t\n", current, tcb);

    /* VU: it is only necessary to notify the other processor if the thread
     * is in one of the scheduling queues (wakeup, ready) or is an
     * interrupt thread */
	thread_state_lock();
    bool need_ipi = tcb->get_state().is_runnable() || 
		(tcb->absolute_timeout != 0) || tcb->is_interrupt_thread();
	thread_state_unlock();

    tcb->set_context(context.root_context());
    

   ASSERT(ALWAYS,!"Migrating domains");
    if (need_ipi)
		xcpu_request(context.root_context(), 
			do_xcpu_put_thread, tcb);
}

#if 0
/**
 * remote handler for tcb_t::migrate_to_processor
 */
static void do_xcpu_set_thread(cpu_mb_entry_t * entry)
{
    if (!entry->tcb)
    {
	enter_kdebug("do_xcpu_set_thread");
	return;
    }
    entry->tcb->migrate_to_domain(entry->param[0]); /* davidm */
}
#endif

bool tcb_t::migrate_to_domain(cpu_context_t context)
{
    ASSERT(DEBUG, this);

    // check if the thread is already on that processor
    if (context.domain == this->get_context().domain)
	return true;

    TRACE_XCPU("CPU%d (%t): %s %t\n", get_current_cpu(), 
	       get_current_tcb(), __func__, this);

    if (this->get_context().domain == get_current_context().domain){
		// thread is on local CPU and should be migrated.
		// Dequeue this tcb, etc
		mp_release_thread(this);

		if (get_current_tcb() == this){
			// if we migrate ourself we use the idle thread to perform 
			// the notification
			get_idle_tcb()->notify(xcpu_put_thread, this, context.raw);
			this->switch_to_idle();
		}else{
			// Otherwise, do it ourselves
			xcpu_put_thread(this, context.raw);
		}
	
    }else{
		UNIMPLEMENTED();
    }
    return true;
}
#endif /* CONFIG_MDOMAINS */

/**
 * Handler invoked when IPC errors occur.  Any * IPC resources are
 * released and control is transferred to user-level.  If the aborted
 * IPC happens to be a pagefault IPC * (caused by user-level memory
 * access) we need to restore the thread * prior to the pagefault
 * and return to user-level.
 */
void handle_ipc_error (void)
{
    tcb_t * current = get_current_tcb ();

    //TRACEF("saved state == %x\n", (word_t)current->get_saved_state ());

    // We're going to skip the last part of the switch_to() function
    // invoked when switching from the current thread.  Make sure that
    // we still manage to load up the appropriate resources when
    // switching to the thread.

    if (EXPECT_FALSE (current->resource_bits))
	current->resources.load (current);

    if (current->get_saved_state ().is_running ())
    {
	// Thread was doing a pagefault IPC.  Restore thread state
	// prior to IPC operation and return directly to user-level.
	for (int i = 0; i < 3; i++)
	    current->set_mr (i, current->misc.ipc_copy.saved_mr[i]);
	current->set_acceptor (current->misc.ipc_copy.saved_acceptor);
	thread_state_lock();
	current->set_partner (current->get_saved_partner ());
	current->set_state (current->get_saved_state ());

	current->set_saved_partner (threadid_t::nilthread ());
	current->set_saved_state (thread_state_t::aborted);
	thread_state_unlock();
	current->return_from_user_interruption ();
    }
    else
    {
	thread_state_lock();
	current->set_saved_state (thread_state_t::aborted); // sanity
	thread_state_unlock();
	current->return_from_ipc ();
    }

    /* NOTREACHED */
}


/* Assumes thread_state_lock is held by caller */
void tcb_t::save_state (void)
{
    for (int i = 0; i < 3; i++)
	misc.ipc_copy.saved_mr[i] = get_mr (i);
    misc.ipc_copy.saved_acceptor = get_acceptor ();
    misc.ipc_copy.saved_error = get_error_code ();
    ASSERT (NORMAL, get_saved_partner () == threadid_t::nilthread ());
    ASSERT (NORMAL, get_saved_state () == thread_state_t::aborted);
    set_saved_partner (get_partner ());
    set_saved_state (get_state ());
}

void tcb_t::restore_state (void)
{
    for (int i = 0; i < 3; i++)
	set_mr (i, misc.ipc_copy.saved_mr[i]);
    set_acceptor (misc.ipc_copy.saved_acceptor);
    set_partner (get_saved_partner ());
    set_state (get_saved_state ());
    set_error_code (misc.ipc_copy.saved_error);

    set_saved_partner (threadid_t::nilthread());
    set_saved_state (thread_state_t::aborted);
}

void tcb_t::send_pagefault_ipc (addr_t addr, addr_t ip,
				space_t::access_e access)
{
    thread_state_lock();
    save_state ();
    thread_state_unlock();

    /* generate pagefault message */
    msg_tag_t tag;
    tag.set(2, IPC_MR0_PAGEFAULT |
	    ((access == space_t::read)      ? (1 << 2) : 0) |
	    ((access == space_t::write)     ? (1 << 1) : 0) |
	    ((access == space_t::execute)   ? (1 << 0) : 0) |
	    ((access == space_t::readwrite) ? (1 << 2)+(1 << 1) : 0), true, true);

    /* create acceptor for whole address space */
    acceptor_t acceptor =  get_acceptor ();

    set_tag(tag);
    set_mr(1, (word_t)addr);
    set_mr(2, (word_t)ip);
    set_acceptor(acceptor);

    //TRACEF("send pagefault IPC (%t)\n", TID(get_pager()));
    do_ipc(get_pager(), get_pager());

    thread_state_lock();
    restore_state ();
    thread_state_unlock();
}

bool tcb_t::send_preemption_ipc(u64_t time)
{
    thread_state_lock();
    save_state ();
    thread_state_unlock();

    /* generate preemption message */
    set_tag (msg_tag_t::preemption_tag());
    set_mr (1, (word_t)time);
    set_mr (2, (word_t)((time >> (BITS_WORD-1)) >> 1)); // Avoid gcc warn
    set_acceptor (acceptor_t::untyped_words()); // only accept untyped words

    /* FIXME: preemption IPC with timeout never -- should be zero */
    msg_tag_t tag = do_ipc(get_scheduler(), get_scheduler());

    thread_state_lock();
    restore_state ();
    thread_state_unlock();

    if (tag.is_error())
    {
	enter_kdebug("preemption IPC error");
    }
    return tag.is_error();
}

/**********************************************************************
 *
 *             global V4 thread management
 *
 **********************************************************************/

tcb_t SECTION(".init") *
create_root_server(threadid_t dest_tid, threadid_t scheduler_tid,
		   threadid_t pager_tid, fpage_t utcb_area,
		   fpage_t kip_area, word_t utcb_address)
{
    ASSERT(DEBUG, dest_tid.is_global());
    ASSERT(DEBUG, scheduler_tid.is_global());
    ASSERT(DEBUG, pager_tid.is_global() || pager_tid.is_nilthread());
    ASSERT(DEBUG, !utcb_area.is_nil_fpage() && !(kip_area.is_nil_fpage()));

    tcb_t * tcb = get_current_space()->get_tcb(dest_tid);
    space_t * space = allocate_space();

    /* VU: we always assume these calls succeed for the root servers
     * if not - we are in deep shit anyway */
    ASSERT(DEBUG, space);
    ASSERT(DEBUG, tcb);

	if(!tcb->create_inactive(dest_tid, scheduler_tid))
		ASSERT(ALWAYS, !"Failed to create root server TCB\n");
    space->init(utcb_area, kip_area);

    /* set the space */
    tcb->set_space(space);
    space->add_tcb(tcb);

    tcb->set_utcb_location (utcb_address);

    /* activate the guy */
    if (!tcb->activate(&thread_return, pager_tid))
	panic("failed to activate root server\n");

    /* and off we go... */
	thread_state_lock();
    tcb->set_state(thread_state_t::running);
	thread_state_unlock();
    get_current_scheduler()->set_priority(tcb, ROOT_PRIORITY);
    get_current_scheduler()->enqueue_ready(tcb);

    return tcb;
}


SYS_THREAD_CONTROL (threadid_t dest_tid, threadid_t space_tid,
		    threadid_t scheduler_tid, threadid_t pager_tid,
		    threadid_t send_redirector, threadid_t recv_redirector,
		    word_t utcb_address)
{
    TRACEPOINT (SYSCALL_THREAD_CONTROL,
		printf ("SYS_THREAD_CONTROL: dest=%t, space=%t, "
			"scheduler=%t, pager=%t, utcb=%x\n",
			TID (dest_tid), TID (space_tid),
			TID (scheduler_tid), TID (pager_tid),
			utcb_address));

    tcb_t * current = get_current_tcb();

    // Check privilege
    if (EXPECT_FALSE (! is_privileged_space(get_current_space())))
    {
	current->set_error_code (ENO_PRIVILEGE);
	return_thread_control(0);
    }

    // Check for valid thread id
    if (EXPECT_FALSE (! dest_tid.is_global() || dest_tid.is_waitnotify()))
    {
	current->set_error_code (EINVALID_THREAD);
	return_thread_control(0);
    }

    tcb_t * dest_tcb = get_current_space()->get_tcb(dest_tid);

    /* interrupt thread id ? */
    if (EXPECT_FALSE (dest_tid.get_threadno() <
		      get_kip()->thread_info.get_system_base()))
    {
	if (EXPECT_TRUE (thread_control_interrupt (dest_tid, pager_tid)))
	    return_thread_control (1);
	/* do not allow the user to mess with kernel threads */
	current->set_error_code (EINVALID_THREAD);
	return_thread_control (0);
    }

    if (space_tid.is_nilthread())
    {
	if (dest_tcb == current)
	{
	    /* do not allow deletion of ourself */
	    current->set_error_code (EINVALID_THREAD);
	    return_thread_control(0);
	}
	else if (dest_tcb->exists())
	{
	    space_t * space = dest_tcb->get_space();
	    dest_tcb->delete_tcb();

	    if (space->remove_tcb(dest_tcb))
	    {
		/* was the last thread */
		space->free();
		free_space(space);
	    }
	}
	else
	{
	    /*
	     * dest_tcb does not exist, trying to create a thread
	     * with space specifier == nilthread
	     */
	    current->set_error_code(EINVALID_SPACE);
	    return_thread_control(0);
	}

	return_thread_control(1);
    }
    else
    {
	/*
	 * thread creation/modification
	 * VU: since we are going to manipulate the thread allocate the tcb
	 * before checking it. If it already exists this is a no-op, otherwise
	 * it saves unmapping the dummy tcb directly afterwards
	 */
	if (!dest_tcb->allocate())
	{
	    current->set_error_code (ENO_MEM);
	    return_thread_control (0);
	}

	/* get the tcb of the space */
	tcb_t * space_tcb = get_current_space()->get_tcb(space_tid);

	if (redirector_loop_detect(dest_tid, recv_redirector))
	{
	    current->set_error_code (EINVALID_REDIRECTOR);
	    return_thread_control (0);
	}

	if (dest_tcb->exists())
	{
	    //TRACEF("thread modification (%t)\n", dest_tcb);
	    // the hardest part first - space modifications
	    if (EXPECT_FALSE (space_tcb->get_global_id() != space_tid))
	    {
		current->set_error_code (EINVALID_SPACE);
		return_thread_control (0);
	    }

	    space_t * space = space_tcb->get_space();
	    if (dest_tcb->get_space() != space)
	    {
		// space migration
		if (EXPECT_FALSE (! dest_tcb->migrate_to_space(space)))
		{
		    current->set_error_code (ENO_MEM);
		    return_thread_control (0);
		}
	    }

	    if (utcb_address != ~0UL)
	    {
		// do not allow modification of UTCB locations of
		// already activated threads
		if (dest_tcb->is_activated() ||
		    ! dest_tcb->check_utcb_location (utcb_address))
		{
		    get_current_tcb ()->set_error_code (EUTCB_AREA);
		    return_thread_control (0);
		}
		dest_tcb->set_utcb_location(utcb_address);
	    }

	    if (!pager_tid.is_nilthread())
	    {
		/* if the thread was inactive, setting the pager will
		 * activate the thread */
		if (!dest_tcb->is_activated())
		{
		    if (! dest_tcb->check_utcb_location ())
		    {
			current->set_error_code (EUTCB_AREA);
			return_thread_control (0);
		    }
		    if (! dest_tcb->activate(&thread_startup, pager_tid))
		    {
			current->set_error_code (ENO_MEM);
			return_thread_control (0);
		    }

		    fake_wait_for_startup (dest_tcb, pager_tid);
		}
		else
		    dest_tcb->set_pager(pager_tid);
	    }

	    if (!scheduler_tid.is_nilthread())
		dest_tcb->set_scheduler(scheduler_tid);
	    if (!send_redirector.is_nilthread())
		dest_tcb->set_send_redirector(send_redirector);
	    if (!recv_redirector.is_nilthread())
		dest_tcb->set_recv_redirector(recv_redirector);


	    // change global id
	    if (dest_tcb->get_global_id() != dest_tid)
		dest_tcb->set_global_id(dest_tid);

	    return_thread_control(1);

	}
	else
	{
	    /* on creation of a new space scheduler must not be nilthread */
	    if (EXPECT_FALSE (scheduler_tid.is_nilthread()))
	    {
		current->set_error_code (EINVALID_SCHEDULER);
		return_thread_control (0);
	    }

	    /* on creation of a new space redirectors must not be nilthread */
	    if (EXPECT_FALSE (send_redirector.is_nilthread() ||
				    recv_redirector.is_nilthread() ))
	    {
		current->set_error_code (EINVALID_REDIRECTOR);
		return_thread_control (0);
	    }

	    /* if the thread is not created in a fresh space
	     * make sure the space id is valid */
	    if (EXPECT_FALSE ((dest_tid != space_tid) &&
			      (space_tcb->get_global_id() != space_tid)))
	    {
		current->set_error_code (EINVALID_SPACE);
		return_thread_control (0);
	    }

	    // For actively created threads, make sure that space is
	    // initialized and UTCB location is valid.
	    if (! pager_tid.is_nilthread ())
	    {
		// Check for initialized space
		if (dest_tid == space_tid ||
		    (! space_tcb->get_space ()->is_initialized ()))
		{
		    current->set_error_code (EINVALID_SPACE);
		    return_thread_control (0);
		}

		// Check for valid UTCB location
		if (utcb_address == ~0UL ||
		    (! space_tcb->check_utcb_location (utcb_address)))
		{
		    current->set_error_code (EUTCB_AREA);
		    return_thread_control (0);
		}
	    }

	    space_t * space;
	    if (dest_tid != space_tid)
		space = space_tcb->get_space();
	    else
		space = allocate_space();

	    /* handle a failing allocate_space */
	    if (!space)
	    {
		current->set_error_code (ENO_MEM);
		return_thread_control (0);
	    }

	    /* ok, try create the thread */
	    if (!dest_tcb->create_inactive(dest_tid, scheduler_tid))
	    {
		if (dest_tid == space_tid)
		    free_space(space);
		current->set_error_code (ENO_MEM);
		return_thread_control (0);
	    }

	    /* set the send and receive redirectors */
	    dest_tcb->set_send_redirector(send_redirector);
	    dest_tcb->set_recv_redirector(recv_redirector);

	    /* set UTCB location, the multiple checks are necessary to
	     * be compliant with the manual which says the operation
	     * succeeds or fails _completely_, do this after
	     * create_inactive -- it overwrites the local_tid */
	    if (utcb_address != ~0UL){
		dest_tcb->set_utcb_location(utcb_address);
	    }

	    /* set the space for the tcb */
	    dest_tcb->set_space (space);
	    space->add_tcb (dest_tcb);

	    /* if pager is not nil the thread directly goes into an IPC */
	    if (! pager_tid.is_nilthread() )
	    {
		if (!dest_tcb->activate(thread_startup, pager_tid))
		{
		    // clean up tcb
		    UNIMPLEMENTED();
		}
		fake_wait_for_startup (dest_tcb, pager_tid);
	    }
	    return_thread_control(1);
	}
    }

    /* NOTREACHED */
    spin_forever ();
}

void SECTION(".init") init_kernel_threads()
{
    // Initialize the user base.  Currently simply leave some space.
    get_kip ()->thread_info.set_user_base
	(get_kip ()->thread_info.get_system_base () + 32);
}

/**
 * initializes the root server
 * Uses the configuration of the kernel interface page and sets up the
 * server.The thread ids of the root server is taken from the kip.
 * Currently, UTCB and KIP area are compile-time constants.
 */
void SECTION(".init") init_root_servers()
{
    TRACE_INIT ("Initializing root servers\n");
    word_t ubase = get_kip()->thread_info.get_user_base();
    tcb_t * tcb;

    fpage_t utcb_area, kip_area;
    word_t size_utcb = 0;


#ifdef CONFIG_DEBUG
    /* Benjl: This is a bit of a hack putting this here */
    strcpy(get_idle_tcb()->debug_name, "idle");
#endif

    /* calculate size of UTCB area for root servers */

    while((1U << size_utcb) < max( get_kip()->utcb_info.get_minimal_size(), 
				   get_kip()->utcb_info.get_utcb_size() * ROOT_MAX_THREADS ))
	size_utcb++;

    utcb_area.set(ROOT_UTCB_START, size_utcb , 0, 0, 0);
    kip_area.set(ROOT_KIP_START, get_kip()->kip_area_info.get_size_log2(),
		 0, 0, 0);

    TRACE_INIT ("root-servers: utcb_area: %p (%dKB), kip_area: %p (%dKB)\n", 
		utcb_area.raw, utcb_area.get_size() / 1024, 
		kip_area.raw, kip_area.get_size() / 1024);

    threadid_t root_server = NILTHREAD;
    root_server.set_global_id(ubase, ROOT_VERSION);

    /* start root task */
    if (!get_kip()->root_server.mem_region.is_empty())
    {
	TRACE_INIT ("creating root server (%t)\n", TID(root_server));
	tcb = create_root_server(
	    root_server,	// tid and space
	    root_server,	// scheduler
	    NILTHREAD,		// pager
	    utcb_area,
	    kip_area,
	    ROOT_UTCB_START);

	tcb->set_user_ip((addr_t)get_kip()->root_server.ip);
	tcb->set_user_sp((addr_t)get_kip()->root_server.sp);
	roottask_space = tcb->get_space();

#ifdef CONFIG_DEBUG
	strcpy(tcb->debug_name, "roottask");
#endif
    } else {
	panic("root task not found!\n");
    }
}
