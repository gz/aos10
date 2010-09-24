/*********************************************************************
 *                
 * Copyright (C) 2002-2003,  Karlsruhe University
 * Copyright (C) 2005,  National ICT Australia (NICTA)
 *                
 * File path:     api/v4/schedule.h
 * Description:   scheduling declarations
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
 * $Id: schedule.h,v 1.22 2003/10/28 10:29:42 joshua Exp $
 *                
 ********************************************************************/

#ifndef __API__V4__SCHEDULE_H__
#define __API__V4__SCHEDULE_H__

#include INC_API(tcb.h)
#include INC_GLUE(schedule.h)
#include <kdb/tracepoints.h>

#include <mp.h>

#define BITMAP_WORDS	( (MAX_PRIO + BITS_WORD) / BITS_WORD )

class prio_queue_t
{
public:
    void enqueue( tcb_t * tcb )
	{
	    ASSERT(DEBUG, tcb != get_idle_tcb());

	    if (tcb->queue_state.is_set(queue_state_t::ready))
		return;

	    ASSERT(DEBUG, tcb->priority >= 0 && tcb->priority <= MAX_PRIO);

	    ENQUEUE_LIST_TAIL (prio_queue[tcb->priority], tcb, ready_list);

	    tcb->queue_state.set(queue_state_t::ready);

	    prio_t prio = tcb->priority;
	    index_bitmap |= (1ul << (prio / BITS_WORD));
	    prio_bitmap[prio / BITS_WORD] |= 1ul << (prio % BITS_WORD);
	}

    void enqueue_head( tcb_t * tcb )
	{
	    ASSERT(DEBUG, tcb != get_idle_tcb());

	    if (tcb->queue_state.is_set(queue_state_t::ready))
		return;

	    ASSERT(DEBUG, tcb->priority >= 0 && tcb->priority <= MAX_PRIO);

	    ENQUEUE_LIST_HEAD (prio_queue[tcb->priority], tcb, ready_list);

	    tcb->queue_state.set(queue_state_t::ready);

	    prio_t prio = tcb->priority;
	    index_bitmap |= (1ul << (prio / BITS_WORD));
	    prio_bitmap[prio / BITS_WORD] |= 1ul << (prio % BITS_WORD);
	}

    void dequeue(tcb_t * tcb)
	{
	    ASSERT(DEBUG, tcb);
	    if (!tcb->queue_state.is_set(queue_state_t::ready))
		return;
	    ASSERT(DEBUG, tcb->priority >= 0 && tcb->priority <= MAX_PRIO);
	    DEQUEUE_LIST(prio_queue[tcb->priority], tcb, ready_list);
	    tcb->queue_state.clear(queue_state_t::ready);
	}

    tcb_t * dequeue_fast(prio_t prio, tcb_t * tcb)
	{
	    ASSERT(DEBUG, tcb);
	    tcb_t *head;
	    tcb->queue_state.clear(queue_state_t::ready);

	    if (tcb->ready_list.next == tcb)
	    {
		head = NULL;
	    }
	    else
	    {
		head = tcb;
		do {
		    head = head->ready_list.next;
		    if (head == tcb) {
			head = NULL;
			goto fast_out;
		    }
		    thread_state_lock();
		    if (!head->get_state().is_runnable()) {
			head->queue_state.clear(queue_state_t::ready);
			thread_state_unlock();
		    } else {
			thread_state_unlock();
			break;
		    }
		} while (1);

		tcb_t * prev = tcb->ready_list.prev;
		prev->ready_list.next = head;
		head->ready_list.prev = prev;
	    }
fast_out:
	    prio_queue[prio] = head;
	    return head;
	}

	tcb_t *get_prio_queue_head(prio_t prio)
	{
	    return prio_queue[prio];
	}

#ifdef CONFIG_MUNITS
	tcb_t * find_smt_runnable(prio_t prio, word_t curr_unit, int *constraint_mismatch)
	{
	    tcb_t *head = prio_queue[prio];
	    tcb_t *tcb_to_run = NULL;
	    tcb_t *curr;

	    /* We start at the head and search for a thread that is not
	     * currently being run and that matches our contraint.
	     * If we dont find one, we return the thread currently being run
	     * that does match our contraint or NULL if it doesn't.
	     */

	    if (head == NULL) {
		return NULL;
	    }

	    curr = head;

	    do {
top:
		thread_state_lock();
		if (!curr->get_state().is_runnable()) {
		    thread_state_unlock();
		    tcb_t *p = curr;
		    curr = curr->ready_list.next;
		    dequeue (p);
		    if (curr == p) {
			/* we removed the head (and only thread) */
			if(tcb_to_run)
			    TRACEF("********************** TCB_to_run != NULL\n");
			return tcb_to_run;
		    }

		    if (p == head) {
			/* removed head of non-empty list */
			head = curr;
		    } else {
			if (curr == head) {
			    /* back at start of the list */
			    break;
			}
		    }

		    goto top;
		}

		if ((curr->get_state().is_runnable()) 
			&& (curr->context_bitmask & curr_unit) ) {

		    /* runnable candidate */
		    if (curr->is_scheduled() == true) {
			if (curr == get_current_tcb()) {
			    /* candidate */
			    tcb_to_run = curr; 
			}

			/* skip it */
		    }  else {
			/* found suitable */
			thread_state_unlock();
			return curr;
		    }
		}
		thread_state_unlock();			
		if ((curr->context_bitmask & curr_unit) == 0) {
		    *constraint_mismatch = 1;
		}

		curr = curr->ready_list.next;

	    } while (curr != head);		


	    /* No match, but test if current thread being scheduled (on our hw
	     * thread) is still runnable 
	     */

	    return tcb_to_run;

	}
#endif


    void set(prio_t prio, tcb_t * tcb)
	{
	    ASSERT(DEBUG, tcb);
	    ASSERT(NORMAL, prio >= 0 && prio <= MAX_PRIO);
	    this->prio_queue[prio] = tcb;
	}

    tcb_t * get(prio_t prio)
	{
	    ASSERT(NORMAL, prio >= 0 && prio <= MAX_PRIO);
	    return prio_queue[prio];
	}

    void init ()
	{
	    word_t i;
	    /* prio queues */
	    for(i = 0; i < MAX_PRIO; i++)
		prio_queue[i] = (tcb_t *)NULL;
	    index_bitmap = 0;
	    for(i = 0; i < BITMAP_WORDS; i++)
		prio_bitmap[i] = 0;
	}

private:
    tcb_t * prio_queue[MAX_PRIO + 1];
public:
    word_t index_bitmap;
    word_t prio_bitmap[BITMAP_WORDS];
    tcb_t * timeslice_tcb;
    s64_t current_timeslice;
};

class scheduler_t
{
    public:
	/**
	 * initializes the scheduler, must be called before init
	 */
	void init( bool bootcpu = true );

	/**
	 * starts the scheduling, does not return
	 * @param cpu processor the scheduler starts on
	 */
	void start(cpu_context_t context);

	/**
	 * schedule a runnable thread
	 * @param current the current thread control block
	 * @return true if a runnable thread was found, false otherwise
	 */
	bool schedule(tcb_t * current);

	/** yield to the highest priority thread
	 * @param current the current thread control block -> to be removed in the future
	 */
	void yield(tcb_t * current);

	/**
	 * activate a thread, if it is highest priority ready thread then
	 * do so immediately, else put it on the run queue
	 */
	void switch_highest(tcb_t *current, tcb_t * tcb);


	/**
	 * delay preemption
	 * @param current   current TCB
	 * @param tcb       destination TCB
	 * @return true if preemption was delayed, otherwise false
	 */
	bool delay_preemption ( tcb_t * current, tcb_t * tcb );


    /**
     * scheduler-specific dispatch decision
     * @param current current thread control block
     * @param tcb tcb to be dispatched
     * @return true if tcb can be dispatched, false if a
     * scheduling decission has to be made
     */
    bool check_dispatch_thread(tcb_t * current, tcb_t * tcb)
	{
	    ASSERT(DEBUG, tcb);
	    ASSERT(DEBUG, current);

	    /* No locking needed as scheduler state is not touched */

	    return (current->priority < tcb->priority);
	}

    /**
     * scheduler preemption accounting remaining timeslice etc.
     */
    void preempt_thread(tcb_t * current, tcb_t * dest)
	{
	    ASSERT(DEBUG, dest);
	    ASSERT(DEBUG, current);
	    ASSERT(DEBUG, current != get_idle_tcb()); // don't "preempt" idle
#ifdef CONFIG_MDOMAINS
	    ASSERT(DEBUG, current->get_context().domain == dest->get_context().domain);
#endif

	    /* re-account the remaining timeslice */
	    scheduler_lock();
	    current->current_timeslice =
		get_prio_queue(current)->current_timeslice;
	    enqueue_ready_head(current);

	    /* now switch to timeslice of dest */
	    get_prio_queue(dest)->current_timeslice = dest->current_timeslice;
	    scheduler_unlock();
	}

	/**
	 * Enqueues tcb into the ready list 
	 * @param tcb thread control block
	 */
	void enqueue_ready(tcb_t * tcb)
	{
	    scheduler_lock();
	    enqueue_ready_prelocked(tcb);
	    scheduler_unlock();
	}

	/**
	 * dequeues tcb from the ready list (if the thread is in)
	 * @param tcb thread control block
	 */
	void dequeue_ready(tcb_t * tcb)
	{
	    scheduler_lock();
	    dequeue_ready_prelocked(tcb);
	    scheduler_unlock();
	}

	/**
	 * sets the priority of a thread
	 * @param tcb thread control block
	 * @param prio priority of thread
	 */
	void set_priority(tcb_t * tcb, prio_t prio)
	{
	    ASSERT(DEBUG, prio >= 0 && prio <= MAX_PRIO);
	    ASSERT(NORMAL, !tcb->queue_state.is_set(queue_state_t::ready));
	    tcb->priority = prio;
	}

	/**
	 * initialize the scheduling parameters of a TCB
	 * @param tcb		pointer to TCB
	 * @param prio		priority of the thread
	 * @param total_quantum	initial total quantum
	 * @param timeslice_length	length of time slice
	 */
	void init_tcb(tcb_t * tcb, prio_t prio = DEFAULT_PRIORITY,
		word_t total_quantum = DEFAULT_TOTAL_QUANTUM,
		word_t timeslice_length = DEFAULT_TIMESLICE_LENGTH,
		word_t context_bitmask = DEFAULT_CONTEXT_BITMASK)
	{
	    scheduler_lock();
	    set_timeslice_length(tcb, timeslice_length);
	    set_total_quantum(tcb, total_quantum);
	    set_priority(tcb, prio);
	    set_context_bitmask(tcb, context_bitmask);
	    scheduler_unlock();
	}

	/**
	 * handles the timer interrupt event, walks the wait lists 
	 * and makes a scheduling decission
	 */
	void handle_timer_interrupt();

	void timer_process_current();
	void hwthread_timer_tick(tcb_t *);
	void process_domain_timer_tick(tcb_t *);

	/**
	 * delivers the current time relative to the system 
	 * startup in microseconds
	 * @return absolute time 
	 */
	u64_t get_current_time()
	{
	    /* no real need to lock here */
	    return current_time;
	}


	/* main part of schedule system call */
	bool do_schedule(tcb_t * tcb, word_t ts_len, word_t total_quantum, word_t prio);

	/* get a fresh timeslice for the given timeslice
	 * ignores any remaining time in this timeslice
	 */
	void fresh_timeslice(tcb_t * tcb)
	{
	    scheduler_lock();
	    get_prio_queue(tcb)->current_timeslice = 0;
	    end_of_timeslice (tcb);
	    scheduler_unlock();
	}

	void scheduler_lock(void) 
	{
#if defined (CONFIG_MUNITS)
	    gs_lock.lock();
#endif
	}

	void scheduler_unlock(void) 
	{
#if defined (CONFIG_MUNITS)
	    gs_lock.unlock();
#endif
	}

#if defined (CONFIG_MUNITS)
	/* update scheduling info on what is currently running */
	void set_scheduled_unit(tcb_t *tcb, cpu_context_t context)
	{
	    /* FIXME: This is <= because of a quirk in context startup
	     * Ie contexts 1-5 are started before 0
	     */
	    ASSERT(ALWAYS, context.unit <= max_execution_unit);
	    hw_scheduled_list[context.unit] = tcb;
	}

	/* query scheduling info on what is currently running */
	tcb_t *get_scheduled_unit(cpu_context_t context)
	{
	    if(context == context.root_context())
		return NULL;
	    ASSERT(ALWAYS, context.unit < max_execution_unit);
	    return hw_scheduled_list[context.unit];
	}

	/* update hint (must hold scheduler lock) */
	void set_unit_hint(tcb_t *tcb, cpu_context_t context)
	{
	    /* FIXME: This is <= because of a quirk in context startup
	     * Ie contexts 1-5 are started before 0
	     */
	    ASSERT(ALWAYS, context.unit <= max_execution_unit);
	    hw_hint_list[context.unit] = tcb;
	}

	/* query hint (must hold scheduler lock) */
	tcb_t *get_unit_hint(cpu_context_t context)
	{
	    if(context == context.root_context())
		return NULL;
	    ASSERT(ALWAYS, context.unit < max_execution_unit);
	    return hw_hint_list[context.unit];
	}

	/* find out which hw thread is running the lowest priority thread for any
	 * hw thread in constraint and priority of a comparator.
	 * Returns possible hwthread candidate to scheduled this on, or current
	 * hwthread (which may be a candidate or not).
	 */
	cpu_context_t lowest_priority_unit(word_t context_bitmask)
	{
	    cpu_context_t candidate = get_current_context().root_context();
	    cpu_context_t i;
	    int candidate_priority = 255;

	    i.domain = candidate.domain;
	    i.unit = 0;

	    for(;i.unit<max_execution_unit;i.unit++) {
		if (context_bitmask & (((word_t)1)<<i.unit)) {
		    int priority = get_scheduled_unit(i)->priority;
		    /* we don't care about the hints priority, as it is either higher or ignored */

		    /* A candidate must have a priority lower than the comparator */
		    if (priority < candidate_priority) {
			candidate = i;
			candidate_priority = priority;
		    }
		}
	    }

	    return candidate;
	}
#endif

	/* if tcb could run on another hw thread, we tell that hw thread to run
	 * schedule.
	 */
	bool smt_reschedule(tcb_t *tcb);

	void scheduler_t::enqueue_ready_prelocked(tcb_t * tcb)
	{
	    ASSERT(DEBUG, tcb);
	    //ASSERT(DEBUG, tcb->get_cpu() == get_current_cpu());
	    get_prio_queue(tcb)->enqueue(tcb);
	}

	void dequeue_ready_prelocked(tcb_t * tcb)
	{
	    ASSERT(DEBUG, tcb);
	    get_prio_queue(tcb)->dequeue(tcb);
	}

	void set_context_bitmask(tcb_t *tcb, word_t x)
	{
#if defined (CONFIG_MUNITS)
	    ASSERT(DEBUG, tcb);
	    tcb->context_bitmask = x;
#endif
	}


    private:
	word_t get_context_bitmask(tcb_t *tcb)
	{
	    ASSERT(DEBUG, tcb);
	    return tcb->context_bitmask;
	}

	/**
	 * function is called when the total quantum of a thread expires 
	 * @param tcb thread control block of the thread whose quantum expired
	 */
	void total_quantum_expired(tcb_t * tcb);

	/**
	 * Marks end of timeslice
	 * @param tcb       TCB of thread whose timeslice ends
	 */
	void end_of_timeslice (tcb_t * tcb);

	/**
	 * hierarchical scheduling prio queue
	 */
	prio_queue_t * get_prio_queue(tcb_t * tcb)
	{
	    ASSERT(DEBUG, tcb);
	    return &this->root_prio_queue;
	}

	void enqueue_ready_head(tcb_t * tcb);


	/**
	 * sets the total time quantum of the thread
	 */
	void set_total_quantum(tcb_t * tcb, word_t quantum)
	{
	    ASSERT(DEBUG, tcb);

	    tcb->total_quantum = quantum;
	    // give a fresh timeslice according to the spec
	    tcb->current_timeslice = tcb->timeslice_length;
	}


	/**
	 * sets the timeslice length of a thread 
	 * @param tcb thread control block of the thread
	 * @param timeslice timeslice length (must be a time period)
	 */
	void set_timeslice_length(tcb_t * tcb, word_t timeslice)
	{
	    ASSERT(DEBUG, tcb);
	    tcb->current_timeslice = tcb->timeslice_length = timeslice;
	}


	/**
	 * searches the run queue for the next runnable thread
	 * @return next thread to be scheduled
	 */
	tcb_t * find_next_thread(prio_queue_t * prio_queue);



	prio_queue_t root_prio_queue;
	static volatile u64_t current_time;
    private:
	/* danielp: lock for global scheduler */
	spinlock_t gs_lock;
#if defined (CONFIG_MUNITS)
	tcb_t * hw_scheduled_list[CONFIG_MAX_UNITS_PER_DOMAIN];
	tcb_t * hw_hint_list[CONFIG_MAX_UNITS_PER_DOMAIN];
	int max_execution_unit;
#endif
};
 




/* global function declarations */

/**
 * @return the current scheduler 
 */
INLINE scheduler_t * get_current_scheduler()
{
    extern scheduler_t scheduler[];
#ifdef CONFIG_MDOMAINS
    return &scheduler[get_current_context().domain];
#else
    return scheduler;
#endif
}

#ifdef CONFIG_MDOMAINS
INLINE scheduler_t * get_scheduler(cpu_context_t context){
    extern scheduler_t scheduler[];
    return &scheduler[context.domain];
}
#endif

/* Get the current l4 scheduler time */
INLINE u64_t get_current_time()
{
    return get_current_scheduler()->get_current_time();
}

#endif /*__API__V4__SCHEDULE_H__*/

