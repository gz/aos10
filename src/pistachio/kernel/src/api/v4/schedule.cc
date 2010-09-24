/*********************************************************************
 *                
 * Copyright (C) 2002-2004,  Karlsruhe University
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *                
 * File path:     api/v4/schedule.cc
 * Description:   Scheduling functions
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
 * $Id: schedule.cc,v 1.56 2004/06/02 08:41:43 sgoetz Exp $
 *                
 ********************************************************************/
#include <l4.h>
#include <debug.h>
#include INC_API(tcb.h)
#include INC_API(schedule.h)
#include INC_API(interrupt.h)
#include INC_API(queueing.h)
#include INC_API(syscalls.h)
#include INC_API(smp.h)
#include INC_API(kernelinterface.h)
#include INC_API(cache.h)
#include INC_GLUE(syscalls.h)
#include INC_GLUE(config.h)
#include INC_GLUE(cache.h)
#include INC_ARCH(special.h)
#include INC_GLUE(intctrl.h)

#include <kdb/tracepoints.h>

#include <mp.h>

#define SCHEDDEBUG(...)
//#define SCHEDDEBUG printf


volatile u64_t scheduler_t::current_time = 0;

#define QUANTUM_EXPIRED (~0UL)

DECLARE_TRACEPOINT(SYSCALL_THREAD_SWITCH);
DECLARE_TRACEPOINT(SYSCALL_SCHEDULE);
DECLARE_TRACEPOINT(TIMESLICE_EXPIRED);
DECLARE_TRACEPOINT(TOTAL_QUANTUM_EXPIRED);
DECLARE_TRACEPOINT(PREEMPTION_FAULT);
DECLARE_TRACEPOINT(PREEMPTION_SIGNALED);


/**
 * sends preemption IPC to the scheduler thread that the total quantum
 * has expired */
void scheduler_t::total_quantum_expired(tcb_t * tcb)
{
    if ( EXPECT_FALSE(tcb->get_preempt_flags().is_signaled()) )
    {
	TRACEPOINT(PREEMPTION_SIGNALED, 
		printf("preemption signalled for %t\n", tcb));

	tcb->set_preempted_ip( tcb->get_user_ip() );
	tcb->set_user_ip( tcb->get_preempt_callback_ip() );
    }

    TRACEPOINT(TOTAL_QUANTUM_EXPIRED, 
	       printf("total quantum expired for %t\n", tcb));
    tcb->send_preemption_ipc(current_time);
}


/**
 * find_next_thread: selects the next tcb to be dispatched
 *
 * returns the selected tcb, if no runnable thread is available, 
 * the idle tcb is returned.
 */
#if defined (CONFIG_MUNITS)
tcb_t * scheduler_t::find_next_thread(prio_queue_t * prio_queue)
{
    ASSERT(DEBUG, prio_queue);

    /* generate bitmask for current context */
    word_t unit = 1 << get_current_context().unit;

    if (prio_queue->index_bitmap) {
	word_t top_word = msb(prio_queue->index_bitmap);
	word_t offset = BITS_WORD * top_word;

	for (long i = top_word; i >= 0; i--)
	{
	    word_t bitmap = prio_queue->prio_bitmap[i];
	    word_t update_bitmap = bitmap;

	    if (bitmap == 0)
		goto update;

	    do {
		word_t bit = msb(bitmap);
		word_t prio = bit + offset;
		tcb_t *tcb = prio_queue->get(prio);

		int constraint_mismatch = 0;

		if (tcb) {
		    ASSERT(NORMAL, tcb->queue_state.is_set(queue_state_t::ready));

		    /* First test if the head is available and matches
		     * contraint */
		thread_state_lock();

		    if ( EXPECT_TRUE(tcb->get_state().is_runnable() 
				     && (tcb->is_scheduled() == false) 
				     && (tcb->context_bitmask & unit))) {
				thread_state_unlock();
found:
			prio_queue->prio_bitmap[i] = update_bitmap;
			if (update_bitmap == 0) {
			    prio_queue->index_bitmap &= ~(1ul<<i);
			}
			
			// don't care about this get_state as it is just printing it out, doesn't really matter what the value is
			SCHEDDEBUG("picked(%d): %t %s\n", unit, tcb, tcb->get_state().string());
			return tcb;
		    } else {
			thread_state_unlock();
			tcb = prio_queue->find_smt_runnable(prio, unit, 
							    &constraint_mismatch);
			if (tcb != NULL) {
			    goto found;
			} 
		    }
		}
		bitmap &= ~(1ul << bit);
		if (constraint_mismatch == 0) {
		    if (prio_queue->get_prio_queue_head(prio) != NULL) {
			
		    } else {
			update_bitmap &= ~(1ul << bit);
		    }
		}
	    } while (bitmap);
	    /* Update the bitmap before looking for more bits */
	    prio_queue->prio_bitmap[i] = update_bitmap;
	    if (update_bitmap == 0) {
		prio_queue->index_bitmap &= ~(1ul<<i);
	    }
update:
	    offset -= BITS_WORD;
	}
    }

    /* if we can't find a schedulable thread - switch to idle */
    return get_idle_tcb();
}
#else
tcb_t * scheduler_t::find_next_thread(prio_queue_t * prio_queue)
{
    ASSERT(DEBUG, prio_queue);

    if (prio_queue->index_bitmap) {
	word_t top_word = msb(prio_queue->index_bitmap);
	word_t offset = BITS_WORD * top_word;

	for (long i = top_word; i >= 0; i--)
	{
	    word_t bitmap = prio_queue->prio_bitmap[i];

	    if (bitmap == 0)
		goto update;

	    do {
		word_t bit = msb(bitmap);
		word_t prio = bit + offset;
		tcb_t *tcb = prio_queue->get(prio);

		if (tcb) {
			thread_state_lock();
		    ASSERT(NORMAL, tcb->queue_state.is_set(queue_state_t::ready));

		    if ( EXPECT_TRUE(tcb->get_state().is_runnable() /*&& tcb->current_timeslice > 0 */) )
		    {
			thread_state_unlock();
found:
			prio_queue->prio_bitmap[i] = bitmap;
			/* TRACE("find_next: returns %t\n", tcb); */
			return tcb;
		    }
		    else {
			/*if (tcb->get_state().is_runnable())
			    TRACEF("dequeueing runnable thread %t without timeslice\n", tcb);*/
			thread_state_unlock();
			/* dequeue non-runnable thread */
			tcb = prio_queue->dequeue_fast(prio, tcb);
			if (tcb)
			    goto found;
		    }
		}
		/* If we get here, there are nomore runnable threads at this
		 * priority. Remove bit from prio_bitmap
		 */
		bitmap &= ~(1ul << bit);
	    } while (bitmap);
	    /* Update the bitmap before looking for more bits */
	    prio_queue->prio_bitmap[i] = 0;
	    prio_queue->index_bitmap &= ~(1ul<<i);

update:
	    offset -= BITS_WORD;
	}
    }

    /* if we can't find a schedulable thread - switch to idle */
    //TRACE("find_next: returns idle\n");
    return get_idle_tcb();
}
#endif

/**
 * selects the next runnable thread and activates it.
 * @return true if a runnable thread was found, false otherwise
 */
bool scheduler_t::schedule(tcb_t * current)
{
    tcb_t * tcb;

    scheduler_lock();
    tcb = find_next_thread (&root_prio_queue);

    ASSERT(DEBUG, current);
    ASSERT(NORMAL, tcb);

    /* do not switch to ourself */
    if (tcb == current)
    {
	scheduler_unlock();
	return false;
    }

    if (current != get_idle_tcb())
	enqueue_ready_prelocked(current);

    tcb->set_scheduled();
    scheduler_unlock();
    current->switch_to(tcb);

    return true;
}

/**
 * activate a thread, if it is highest priority ready thread then
 * do so immediately, else put it on the run queue
 */
void scheduler_t::switch_highest(tcb_t *current, tcb_t * tcb)
{
    tcb_t * max;

    scheduler_lock();
    max = find_next_thread (&root_prio_queue);

    ASSERT(DEBUG, tcb);
    ASSERT(NORMAL, max);

    /* no locking used in check_dispatch */
    if (check_dispatch_thread (max, tcb) )
    {
#if defined (CONFIG_MUNITS)
	SMT_ASSERT(ALWAYS, tcb->is_scheduled() == false);
#endif
	tcb->set_scheduled();
	thread_state_lock();
	tcb->set_state( thread_state_t::running );
	thread_state_unlock();
	scheduler_unlock();
	current->switch_to(tcb);
    }
    else
    {
	thread_state_lock();
	tcb->set_state( thread_state_t::running );
	thread_state_unlock();
	SMT_ASSERT(ALWAYS, tcb->is_scheduled() == false);
	max->set_scheduled();
	enqueue_ready_prelocked(tcb);
	scheduler_unlock();
	smt_reschedule(tcb);
	current->switch_to(max);
    }
}


/**
 * Ensure current's state is not runnble() before this call!
 */
void scheduler_t::yield(tcb_t * current)
{
    tcb_t * tcb;

    scheduler_lock();
    tcb = find_next_thread (&root_prio_queue);

    ASSERT(NORMAL, tcb);
    /* if the following assert fires on SMT may need to set tcb to idle_tcb here */
    ASSERT(NORMAL, tcb != current);

    tcb->set_scheduled();
    scheduler_unlock();

    current->switch_to(tcb);
}


/**
 * selects the next runnable thread in a round-robin fashion
 * For SMT systems, scheduler is assumed to be locked before function entry
 */
void scheduler_t::end_of_timeslice (tcb_t * tcb)
{
    ASSERT(DEBUG, tcb);
    ASSERT(DEBUG, tcb != get_idle_tcb()); // the idler never yields

    prio_queue_t * prio_queue = get_prio_queue ( tcb );
    ASSERT(DEBUG, prio_queue);

#if !defined (CONFIG_MUNITS)
    /* make sure we are in the run queue, put the TCB in front to
     * maintain the RR order */
    enqueue_ready_head (tcb);

    /* now schedule RR */
    prio_queue->set(tcb->priority, tcb->ready_list.next);


    // renew timeslice
    tcb->current_timeslice = prio_queue->current_timeslice + tcb->timeslice_length;

    /* now give the prio queue the newly scheduled thread's timeslice;
     * if we are the only thread in this prio we are going to be
     * rescheduled */
    prio_queue->current_timeslice = prio_queue->get(tcb->priority)->current_timeslice;
#else
    /* make sure we are in the run queue, put the TCB at end to
     * maintain the RR order between multiple SMT threads */
    dequeue_ready_prelocked(tcb); // make sure it isn't in list
    enqueue_ready_prelocked(tcb); // place it at end of list

    // renew timeslice
    tcb->current_timeslice = prio_queue->current_timeslice + tcb->timeslice_length;

    prio_queue->current_timeslice = prio_queue->get(tcb->priority)->current_timeslice;
#endif

}


void scheduler_t::handle_timer_interrupt()
{
    kdebug_check_breakin();

#if defined(CONFIG_MUNITS) || defined(CONFIG_MDOMAINS)
    if (get_current_context().raw == 0)
#endif
    {
	/* update the time global time*/
	current_time += get_timer_tick_length();
    }
    SMT_ASSERT (ALWAYS, get_current_context().unit == 0);


    tcb_t * current = get_current_tcb();


#ifdef CONFIG_MUNITS
    process_domain_timer_tick();
#endif

    /* the idle thread schedules itself so no point to do it here.
     * Furthermore, it should not be preempted on end of timeslice etc.
     */
    if (current == get_idle_tcb())
    {
	return;
    }

    scheduler_lock();

    /* Check for not infinite timeslice and expired */
    if ( EXPECT_TRUE( current->timeslice_length != 0 ) &&
	 EXPECT_FALSE( (current->current_timeslice -=
			 get_timer_tick_length()) <= 0 ) )
    {
	// We have end-of-timeslice.
	TRACEPOINT(TIMESLICE_EXPIRED, 
		    printf("timeslice expired for %t\n", current));

	end_of_timeslice ( current );
    }
    else
    {
	if ( EXPECT_FALSE(current->get_preempt_flags().is_signaled()) )
	{
	    TRACEPOINT(PREEMPTION_SIGNALED, 
		    printf("preemption signalled for %t\n", current));

	    current->set_preempted_ip( current->get_user_ip() );
	    current->set_user_ip( current->get_preempt_callback_ip() );
	}

	scheduler_unlock();
	return;
    }

    scheduler_unlock();
    timer_process_current();
}


#if defined (CONFIG_MUNITS)
static void do_smt_timer_tick(cpu_mb_entry_t *entry)
{
    tcb_t *current = get_current_tcb();

    /* The idle thread schedules itself so no point to do it here.
     * Furthermore, it should not be preempted on end of timeslice etc.
     */
    if (current == get_idle_tcb())
    {
	return;
    }

    get_current_scheduler()->hwthread_timer_tick(current);

}

void scheduler_t::process_domain_timer_tick()
{
    int sent = 0;
    cpu_context_t i = get_current_context();
    cpu_context_t j = get_current_context();
    /* 
     * (Daniel) This is done in up to two stages.
     *  1. If any running threads are at a lower priority than our primary
     *     wakeup thread, we tell all hardware threads to re-schedule.
     *     XXX This will go away when timeouts are removed XXX
     *  2. If we haven't woken up our threads already, we then check if any
     *     need to reschedule because the thread they are running has a
     *     timeslice expiry. If so, we tell that hardware thread to
     *     reschedule. Also, this stage ensures we update the timeslice
     *     available to each thread (so we MUST execute this stage).
     */

    scheduler_lock();
#if 0
    for(i.unit=1;i.unit<max_execution_unit;i.unit++) 
    {
	tcb_t * tcb = get_scheduled_unit(i);

	if (tcb && (tcb->priority < wakeup->priority)) 
	{
	    /* Wakeup thread will preempt at least one hwthread (so wake up all) */
	    for(j.unit=1;j.unit<max_execution_unit;j.unit++) {
		scheduler_unlock();
		xcpu_request(j, do_smt_timer_tick);
		scheduler_lock();
	    }
	    sent = 1;
	}
    }
#endif
    /* Stage 2 (see comment above) */

    for(j.unit=1;j.unit<max_execution_unit;j.unit++) 
    {
	tcb_t * tcb = get_scheduled_unit(j);
	if (tcb) {
	    if ( EXPECT_TRUE( tcb->timeslice_length != 0 ) &&
		 EXPECT_FALSE( (tcb->current_timeslice -=
				get_timer_tick_length()) <= 0 ) && (sent == 0))
	    {
		// We have end-of-timeslice.
		if (sent == 0) {
		    scheduler_unlock();
		    xcpu_request(j, do_smt_timer_tick);
		    scheduler_lock();
		}
	    }
	}
    }

    scheduler_unlock();
}


void scheduler_t::hwthread_timer_tick(tcb_t *current)
{
    /* 
     * (Daniel) This function processes a timer tick event for a particular
     * hardware thread. We confirm that we need to reschedule due to timeslice
     * expiry. Note that the timeslice for the current running thread has
     * already been updated by the original timer tick recipient.
     */

    scheduler_lock();

    /* Check for not infinite timeslice and expired */
    if ( EXPECT_TRUE( current->timeslice_length != 0 ) &&
	 EXPECT_FALSE( (current->current_timeslice) <= 0 ) )
    {
	// We have end-of-timeslice.
	TRACEPOINT(TIMESLICE_EXPIRED, 
		    printf("timeslice expired for %t\n", current));

	end_of_timeslice ( current );
	scheduler_unlock();
	timer_process_current();
    } else {
	scheduler_unlock();
    }
}
#endif


void scheduler_t::timer_process_current()
{
    tcb_t * current = get_current_tcb();

    /* time slice expired */
    if (EXPECT_FALSE (current->total_quantum))
    {
	/* we have a total quantum - so do some book-keeping */
	if (current->total_quantum == QUANTUM_EXPIRED)
	{
	    /* VU: must be revised. If a thread has an expired time quantum
	     * and is activated with switch_to his timeslice will expire
	     * and the event will be raised multiple times */
	    total_quantum_expired (current);
	}
	else if (current->total_quantum <= current->timeslice_length)
	{
	    /* we are getting close... */
	    current->current_timeslice += current->total_quantum;
	    current->total_quantum = QUANTUM_EXPIRED;
	}
	else
	{
	    // account this time slice
	    current->total_quantum -= current->timeslice_length;
	}
    }

    /* schedule the next thread */
    enqueue_ready(current);

    if ( schedule(current) )
    {
	if ( EXPECT_FALSE(current->get_preempt_flags().is_signaled()) )
	{
	    TRACEPOINT(PREEMPTION_SIGNALED, 
		    printf("preemption signalled for %t\n", current));

	    current->set_preempted_ip( current->get_user_ip() );
	    current->set_user_ip( current->get_preempt_callback_ip() );
	}
    }
}


void init_all_threads(void)
{
    init_interrupt_threads();
    init_kernel_threads();
    init_root_servers();

#ifdef CONFIG_KDB_ON_STARTUP
	enter_kdebug("startup system");
#endif
}
/**
 * the idle thread checks for runnable threads in the run queue
 * and performs a thread switch if possible. Otherwise, it 
 * invokes a system sleep function (which should normally result in
 * a processor halt)
 */
void idle_thread()
{
#if defined (CONFIG_MDOMAINS) || defined (CONFIG_MUNITS)
    TRACE_INIT("idle thread started on CPU (%d, %d), priority %d\n", get_current_context().domain, get_current_context().unit, get_idle_tcb()->priority);
#else
    TRACE_INIT("idle thread started\n");
#endif

    cache_t::flush_range_attribute(get_kip(),
	    (addr_t)((word_t)get_kip() + get_kip_size()), attr_clean_d);

    while(1)
    {
	if (!get_current_scheduler()->schedule(get_idle_tcb()))
	{
	    processor_sleep();
	}
    }
}

SYS_THREAD_SWITCH (threadid_t dest)
{
    /* Make sure we are in the ready queue to 
     * find at least ourself and ensure that the thread 
     * is rescheduled */
    tcb_t * current = get_current_tcb();
    scheduler_t * scheduler = get_current_scheduler();

    TRACEPOINT( SYSCALL_THREAD_SWITCH,
		printf("SYS_THREAD_SWITCH current=%t, dest=%t\n",
		       current, TID(dest)) );
    

    /* explicit timeslice donation */
    if (!dest.is_nilthread())
    {
	tcb_t * dest_tcb = get_current_space()->get_tcb(dest);

	if ( dest_tcb == current )
	{
	    return_thread_switch();
	}

	/* XXX TODO: SMT check constraint and not running (is_running)!
	 *  and not scheduled..
	 */

	scheduler->scheduler_lock();
	thread_state_lock();
	if ( dest_tcb->get_state().is_runnable() &&
#if defined (CONFIG_MUNITS)
	     (dest_tcb->is_scheduled() == false) && 
#endif
	     dest_tcb->myself_global == dest &&
	     dest_tcb->is_local_domain() )
	{
	    dest_tcb->set_scheduled();
	    thread_state_unlock();
	    scheduler->scheduler_unlock();
	    scheduler->enqueue_ready(current);
	    current->switch_to(dest_tcb);
	    return_thread_switch();
	}

	thread_state_unlock();
	scheduler->scheduler_unlock();
    }

    scheduler->enqueue_ready(current);

    /* eat up timeslice - we get a fresh one */
    scheduler->fresh_timeslice(current);
    scheduler->schedule (current);
    return_thread_switch();
}


/* local part of schedule */
bool scheduler_t::do_schedule(tcb_t * tcb, word_t ts_len, word_t total_quantum, word_t prio)
{
//    ASSERT(DEBUG, tcb->is_local());


    if ( (prio != (~0UL)) && ((prio_t)prio != tcb->priority) )
    {
	dequeue_ready_prelocked(tcb);
	set_priority(tcb, (prio_t) (prio & 0xff));
	enqueue_ready_prelocked(tcb);
    }

    if ( total_quantum != (~0UL) )
    {
	if ( total_quantum == 0 )
	{
	    tcb->total_quantum = QUANTUM_EXPIRED;
	} else
	{
	    set_total_quantum (tcb, total_quantum);
	}
	enqueue_ready_prelocked (tcb);
    }

    if ( ts_len != (~0UL) )
    {
	set_timeslice_length (tcb, ts_len);
    }

    return true;
}

#if defined (CONFIG_MDOMAINS)
static void do_xcpu_schedule(cpu_mb_entry_t * entry)
{
    tcb_t * tcb = entry->tcb;
    
    scheduler_t *scheduler = get_current_scheduler();
    scheduler->scheduler_lock();

#if 0 // XXX We bomb out and do the schedule anyway.
    /* XXX This techinque doesn't work well for SMT as the thread appears to
     * move around frequently.
     */
    // meanwhile migrated? 
    if (!tcb->is_local()) {
	scheduler->scheduler_unlock();
	ASSERT(DEBUG, !"yuck");
	xcpu_request (tcb->get_context(), do_xcpu_schedule, tcb, 
		      entry->param[0], entry->param[1],
		      entry->param[2], entry->param[3]);
	return;
    } 
#endif
    scheduler->do_schedule (tcb, entry->param[0], entry->param[1],
					  entry->param[2], entry->param[3]);
    scheduler->scheduler_unlock();
}
#endif

#if defined (CONFIG_MUNITS)
static void do_xcpu_reschedule(cpu_mb_entry_t * entry)
{
    tcb_t * current = get_current_tcb();
    get_current_scheduler()->scheduler_lock();
    thread_state_lock();
    tcb_t * hint = get_current_scheduler()->get_unit_hint(get_current_context());
    if(hint && !hint->is_scheduled() && hint->get_state().is_runnable())
    {
	if(hint->priority > current->priority)
	{
	    hint->set_scheduled();
	    get_current_scheduler()->set_unit_hint(NULL, get_current_context());
	    thread_state_unlock();
	    get_current_scheduler()->scheduler_unlock();
	    current->switch_to(hint);
	}
	else
	{
	    get_current_scheduler()->set_unit_hint(NULL, get_current_context());
	    thread_state_unlock();
	    get_current_scheduler()->scheduler_unlock();
	    get_current_scheduler()->smt_reschedule(hint);
	}
    }
    else
    {
	get_current_scheduler()->set_unit_hint(NULL, get_current_context());
	thread_state_unlock();
	get_current_scheduler()->scheduler_unlock();
    }
}

static void
do_smt_reschedule(tcb_t *tcb)
{
    scheduler_t *scheduler = get_current_scheduler();

    scheduler->smt_reschedule(tcb);
}


bool scheduler_t::smt_reschedule(tcb_t *tcb)
{
    ASSERT(ALWAYS, tcb != NULL);
    scheduler_lock();
    cpu_context_t context = lowest_priority_unit(tcb->context_bitmask);
    tcb_t * hint = get_unit_hint(context);
    if (context != context.root_context() && context != get_current_context() && 
	    get_scheduled_unit(context)->priority < tcb->priority && 
	    (!hint || hint->priority < tcb->priority)) {
	set_unit_hint(tcb, context);
	scheduler_unlock();
	xcpu_request(context, do_xcpu_reschedule);
	return 0;
    }
    scheduler_unlock();
    return 1; 	/* else we make a local decision */
}
#else
bool scheduler_t::smt_reschedule(tcb_t *tcb)
{
    return 1;
}
#endif

SYS_SCHEDULE (threadid_t dest_tid, word_t ts_len, word_t total_quantum,
	word_t context_bitmask, word_t domain_control, word_t prio )
{
    tcb_t * current = get_current_tcb();

    TRACEPOINT(SYSCALL_SCHEDULE,
	printf("SYS_SCHEDULE: curr=%t, dest=%t, quantum=%x, "
		   "hwtid mask = %08lx, "
	       "timeslice=%x, domain_ctrl=%x, prio=%x\n",
	       current, TID(dest_tid), total_quantum, context_bitmask, ts_len,
	       domain_control, prio));

    tcb_t * dest_tcb = get_current_space()->get_tcb(dest_tid);

    // make sure the thread id is valid
    if (dest_tcb->get_global_id() != dest_tid)
    {
	current->set_error_code (EINVALID_THREAD);
	return_schedule(0, 0, 0);
    }

    // are we in the same address space as the scheduler of the thread?
    tcb_t * sched_tcb = get_current_space()->get_tcb(dest_tcb->get_scheduler());
    if (sched_tcb->get_global_id() != dest_tcb->get_scheduler() ||
	sched_tcb->get_space() != get_current_space())
    {
	current->set_error_code (ENO_PRIVILEGE);
	return_schedule(0, 0, 0);
    }

    scheduler_t *scheduler = get_current_scheduler();

    if ( dest_tcb->is_local_domain() )
    {
	word_t old_bm = dest_tcb->context_bitmask;
	
	if(dest_tcb != get_current_tcb())
	    dest_tcb->pause();
	
	scheduler->scheduler_lock();
	scheduler->do_schedule ( dest_tcb, ts_len, total_quantum, prio);
	scheduler->scheduler_unlock();

	if ( context_bitmask != (~0UL) ) 
	{
	    scheduler->set_context_bitmask(dest_tcb, context_bitmask);
#ifdef CONFIG_MUNITS
	    if(dest_tcb->is_interrupt_thread()){
		get_interrupt_ctrl()->hwthread_mask(context_bitmask, dest_tcb->get_global_id().get_threadno()); 
	    }
#endif
	}
	else
	{
	    scheduler->set_context_bitmask(dest_tcb, old_bm);
	}

	if ( domain_control != ~0UL && 
		 domain_control < (word_t)get_mp()->get_num_scheduler_domains())
	{
#ifdef CONFIG_MDOMAINS
		cpu_context_t to_context;
		to_context.domain = domain_control;
		dest_tcb->migrate_to_domain (to_context.root_context());
#endif
	}
    }
    /* FIXME: this stuff needs to be updated */
    else
    {
	UNIMPLEMENTED();
#if 0
	scheduler->scheduler_unlock();

	xcpu_request(dest_tcb->get_context(), do_xcpu_schedule, 
		     dest_tcb, ts_len, total_quantum, prio, context_bitmask);

	if ( domain_control != ~0UL ) 
	{
	    cpu_context_t to_context;
	    to_context.domain = domain_control;
	    dest_tcb->migrate_to_domain (to_context.root_context());
	}
#endif
    }

#ifdef CONFIG_MUNITS
    // ensure that the thread is running on a valid hw_thread
    if(get_current_tcb() == dest_tcb)
    {
	if((dest_tcb->context_bitmask & (1 << get_current_context().unit))  == 0)
	{
	    scheduler->enqueue_ready(dest_tcb);
	    get_idle_tcb()->notify(do_smt_reschedule, dest_tcb);
	    dest_tcb->switch_to_idle();
	}
    }
    else
    {
	scheduler->smt_reschedule(dest_tcb);
    }
#endif

    thread_state_lock();
    thread_state_t state = dest_tcb->get_state();
    thread_state_unlock();
    word_t result = ((state == thread_state_t::aborted)    ? 1 :
		     state.is_halted()                   ? 2 :
		     state.is_running()                  ? 3 :
		     state.is_polling()                  ? 4 :
		     state.is_sending()                  ? 5 :
		     state.is_waiting()                  ? 6 :
		     state.is_locked_waiting()           ? 7 :
		     state.is_waiting_notify()           ? 8 :
		     state.is_xcpu_waiting()             ? 9 :
		     0);

    if (result == 0) {
	TRACEF("invalid state (%x)\n", (word_t)state);
    }

    return_schedule(result, dest_tcb->current_timeslice,
        dest_tcb->total_quantum);
}


void scheduler_t::enqueue_ready_head(tcb_t * tcb)
{
    ASSERT(DEBUG, tcb);
//    ASSERT(DEBUG, tcb->get_context() == get_mp()->get_current_context());
    get_prio_queue(tcb)->enqueue_head(tcb);
}

/**********************************************************************
 *
 *                     Initialization
 *
 **********************************************************************/

void SECTION(".init") scheduler_t::start(cpu_context_t context)
{
#if defined (CONFIG_MDOMAINS) || defined (CONFIG_MUNITS)
    TRACE_INIT ("Switching to idle thread (Context %d)\n", context.raw);
#else
    TRACE_INIT ("Switching to idle thread\n");
#endif

#ifdef CONFIG_MUNITS
    max_execution_unit++;
    get_idle_tcb()->set_scheduled();
    set_scheduled_unit(get_idle_tcb(), get_current_context());
    set_unit_hint(NULL, get_current_context());
#endif
    
    initial_switch_to(get_idle_tcb());
}

void SECTION(".init") scheduler_t::init( bool bootcpu )
{
//    TRACE_INIT ("\tInitializing threading\n");  <-- not really, anymore
    
#ifdef CONFIG_MUNITS
    max_execution_unit = 0;
#endif

    root_prio_queue.init();
}
