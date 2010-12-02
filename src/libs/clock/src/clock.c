/**
 * Clock Driver
 * =============
 * This files contains the function to handle our timer register
 * and the system call handlers for sleep and time_stamp.
 * For the time_stamp measurement we use the time stamp timer register
 * of the IXP42X Hardware and for sleep calls we use the
 * General-Purpose Timer 0.
 *
 * A sleep system call basically uses the register timer function
 * of the clock driver interface. Register timer will insert
 * an alarm_timer struct into a priority queue and reset the timer
 * if we have a new alarm_timer in the front of the queue.
 * The General-Purpose Timer 0 is programmed to give an interrupt
 * whenever it reaches zero. In that case timer0_irq is called
 * as the interrupt handler. The alarm_timer has an alarm_function
 * which is called by the interrupt handler when the alarm is triggered.
 * Currently we use only wakeup since we only register timers for sleep
 * calls. But the way we implemented it, we could extend it easily.
 *
 * The timestamp timer is started in start_timer and interrupts every time
 * its register overflows. We maintain a current_timestamp_high value where
 * we basically add the ticks of MAXINT converted to microseconds every time
 * a interrupt happens.
 *
 */

#include <stdlib.h>
#include <assert.h>
#include <l4/thread.h>
#include <l4/misc.h>
#include <l4/ipc.h>
#include <l4/space.h>
#include <sos_shared.h>
#include "clock.h"
#include "nslu2.h"
#include "../../../sos/libsos.h"

#define verbose 0

// Internal Driver Functions
static timestamp_t current_timestamp_high = 0ULL;
static alarm_timer* timer_queue_head = NULL;
static L4_Bool_t driver_initialized = FALSE;

// Register Mapping
static L4_ThreadId_t timestamp_irq_tid;
static L4_ThreadId_t timer0_irq_tid;
static L4_Fpage_t registers_fpage;

// Timer Register Addresses
#define OST_TS (NSLU2_OSTS_PHYS_BASE + 0x0)
#define OST_TIM0 (NSLU2_OSTS_PHYS_BASE + 0x4)
#define OST_TIM0_RL (NSLU2_OSTS_PHYS_BASE + 0x8)
#define OST_STATUS (NSLU2_OSTS_PHYS_BASE + 0x20)

// Macros to control Timer 0
#define TIMER0_SET(count)	((*(L4_Word_t*)OST_TIM0_RL)  = ((count) & ~0x3) | 0x2 )
#define TIMER0_START()		((*(L4_Word_t*)OST_TIM0_RL) |= 0x1)
#define TIMER0_STOP()		((*(L4_Word_t*)OST_TIM0_RL)  = (*(L4_Word_t*)OST_TIM0_RL) & ~0x1)
#define TIMER0_ONE_SHOT(v) 	((*(L4_Word_t*)OST_TIM0_RL) |= (v) << 1)

// Macros to construct 64bit timestamp from 32bit timer
#define GET_LOW(timestamp) ( ((L4_Word_t)(timestamp)) & 0xFFFFFFFF)
#define GET_HIGH(timestamp) ((timestamp) & 0xFFFFFFFF00000000)


/**
 * Inserts an alarm_timer in the priority queue. Queue
 * is sorted based on the expiration time. Lower
 * expiration time before higher.
 *
 * @param new_timer the timer to insert
 */
void timer_queue_insert(alarm_timer* new_timer) {
	assert(new_timer != NULL);

	if(timer_queue_head == NULL) {
		timer_queue_head = new_timer;
		return;
	}

	alarm_timer** atp = (alarm_timer**) &timer_queue_head;
	for (; *atp != NULL; atp = &(*atp)->next_alarm) {
		 if (new_timer->expiration_time < (*atp)->expiration_time) break;
	}
	// insert new_timer in the middle or at the end of the list
	new_timer->next_alarm = *atp;
	*atp = new_timer; // set the previous next_alarm pointer to our new inserted timer
}


/**
 * Removes the top element from the timer queue.
 *
 * @return The top element in the timer queue. This is the one
 * with the lowest expiration_time.
 */
alarm_timer* timer_queue_pop() {

	if(timer_queue_head != NULL) {
		alarm_timer* old_head = timer_queue_head;
		timer_queue_head = timer_queue_head->next_alarm;
		return old_head;
	}
	else
		return NULL;

}


/**
 * Starts the clock driver. This will map the memory region where the
 * registers are in uncached mode, start the time stamp timer register
 * and enable the interrupts for our time stamp timer and the general
 * purpose timer 0.
 *
 * @return CLOCK_R_OK if the timer is started successfully
 * 		   CLOCK_R_FAIL if the memory region could not be mapped
 */
int start_timer(void) {
	assert(!driver_initialized);

	// initialize variables
	timestamp_irq_tid =  L4_GlobalId(NSLU2_TIMESTAMP_IRQ, 1);
	timer0_irq_tid = L4_GlobalId(NSLU2_TIMER0_IRQ, 1);
	registers_fpage = L4_FpageLog2(NSLU2_OSTS_PHYS_BASE, 12);

	// Set up uncached memory mapping for registers
	L4_Set_Rights(&registers_fpage, L4_FullyAccessible);
	L4_PhysDesc_t phys = L4_PhysDesc(NSLU2_OSTS_PHYS_BASE, L4_UncachedMemory);

	if(L4_MapFpage(L4_Pager(), registers_fpage, phys)) {

		// enable timer0 interrupts
		TIMER0_ONE_SHOT(0);
		TIMER0_STOP();
		(*(L4_Word_t*)OST_STATUS) |= (0x1 << 0);
		int res = L4_AssociateInterrupt(timer0_irq_tid, root_thread_g);
		assert(res);

		// start timestamp timer
		*((L4_Word_t*)OST_TS) = 0x00000000; // reset counter

		// enable timestamp interrupts
		(*(L4_Word_t*)OST_STATUS) |= (0x1 << 2);
		res = L4_AssociateInterrupt(timestamp_irq_tid, root_thread_g);
		assert(res);

		driver_initialized = TRUE;

		return CLOCK_R_OK;
	}
	else {
		return CLOCK_R_FAIL;
	}
}


/**
 * Interrupt handler for our timestamp timer. This Interrupt happens every time
 * the timer register overflows. In that case we update our current_timestamp_high
 * value and mask the interrupt. This function also sets the IPC message
 * to return to the interrupt thread.
 *
 * @param tid Interrupt handler Thread id
 * @param msg_p IPC message
 * @return 1 to tell the syscall loop to send a reply
 */
int timer_overflow_irq(L4_ThreadId_t tid, L4_Msg_t* msg_p) {
	assert( ((*(L4_Word_t*)OST_STATUS) >> 2) & 0x1); // this must only be called during interrupt

	current_timestamp_high += TICKS_TO_MICROSECONDS(0xFFFFFFFF);
	(*(L4_Word_t*)OST_STATUS) |= (0x1 << 2); // clear the timestamp timer interrupt bit

	return set_ipc_reply(msg_p, 0); // return to interrupt thread with 0 msg
}


/**
 * General Purpose Timer 0 Interrupt handler. This Interrupt happens when
 * we can handle our alarm_timer in the front of our queue.
 * In this interrupt we also check for follwoing timers which will be due within
 * a certain range and call their alarm_function.
 * This function also frees the allocated alarm_timer memory whenever they are
 * removed from the queue.
 *
 * @param tid Interrupt Thread ID
 * @param msg_p IPC message
 * @return 1 because we wan't to send a reply in the syscall loop
 */
int timer0_irq(L4_ThreadId_t tid, L4_Msg_t* msg_p) {
	assert(L4_IsNilThread(tid) || ((*(L4_Word_t*)OST_STATUS)) & 0x1); // this must only be called during interrupt
	assert(timer_queue_head != NULL);

	// set timer to next alarm or if they're already due we call their alarm function
	while(timer_queue_head != NULL) {
			timestamp_t delay = timer_queue_head->expiration_time - time_stamp();

			if( ((int64_t) delay) <= 500) { // TODO: is 500 a good value?
				alarm_timer* alarm = timer_queue_pop();
				alarm->alarm_function(alarm->owner, CLOCK_R_OK);
				free(alarm);
			}
			else {
				// set alarm and return
				dprintf(0, "Next alarm in: %lld us, register is:0x%X\n", delay, (*(L4_Word_t*)OST_TIM0_RL) );

				TIMER0_SET(MICROSECONDS_TO_TICKS(delay));
				TIMER0_START();
				break;
			}

	}
	if (L4_IsNilThread(tid))
		return 0; // don't interfere with ipc, because the method was not called as interrupt handler
	else {
		(*(L4_Word_t*)OST_STATUS) |= 0x1; // clear the timestamp timer interrupt bit
		return set_ipc_reply(msg_p, 0); // return to interrupt thread with 0 msg
	}
}



/**
 * Alarm timer function for sleep calls (wakes up the corresponding thread).
 *
 * @param tid Client thread ID to wakeup
 * @param status The status sent back to the client this will usually be CLOCK_R_OK
 * but if timer_stop() is called before sleep returns CLOCK_R_CNCL is sent.
 * However currently this is not checked in the client (because of the prototype of
 * sleep).
 */
static void wakeup(L4_ThreadId_t tid, int status) {
	dprintf(1, "Alarm Timer from:0x%X is ringing with status:%d\n", tid, status);
	send_ipc_reply(tid, SOS_SLEEP, 1, status);
}


/**
 * Inserts a new alarm timer in the timer queue. This will also restart
 * the timer whenever we have a new head element.
 * Notice that this will construct a alarm_timer struct and set its
 * alarm_function to point to wakeup. Since we only support sleep
 * right now this is ok.
 *
 * @param delay microseconds when the timer should be triggered
 * @param client which user should be triggered
 * @return CLOCK_R_OK if the timer driver is started
 * 		   CLOCK_R_UNIT if the driver is not started yet
 * 		   CLOCK_R_FAIL if we don't have memory for our alarm_timer
 */
int register_timer(uint64_t delay, L4_ThreadId_t client) {
	if(!driver_initialized)
		return CLOCK_R_UINT;

	alarm_timer* new_alarm = malloc( sizeof(alarm_timer) ); // free'd in interrupt handler
	if(new_alarm == NULL)
		return CLOCK_R_FAIL;

	new_alarm->alarm_function = &wakeup;
	new_alarm->owner = client;
	new_alarm->next_alarm = NULL;
	new_alarm->expiration_time = time_stamp() + delay;

	//dprintf(0, "new_alarm->expiration_time:%llu\ntimer_queue_head->expiration_time:%llu\n", new_alarm->expiration_time, timer_queue_head->expiration_time);
	// disable interrupts while modifying the queue
	L4_DeassociateInterrupt(timer0_irq_tid);
	timer_queue_insert(new_alarm);
	L4_AssociateInterrupt(timer0_irq_tid, root_thread_g);

	if(timer_queue_head == new_alarm) {
		TIMER0_SET(MICROSECONDS_TO_TICKS(delay));
		TIMER0_START();

		dprintf(1, "register_timer: new front timer: Next alarm in: %lld us\n", delay);
	}

	return CLOCK_R_OK;
}


/**
 * Removes all currently registred timers for a given thread.
 * @param tid
 * @return
 */
void remove_timers(L4_ThreadId_t tid) {
	// queue is empty, nothing to do
	if(timer_queue_head == NULL) {
		return;
	}

	// if the queue head belongs to this tid, simulate timer interrupt
	// do this as long as the new queue head also belongs to this tid
	while (L4_IsThreadEqual(timer_queue_head->owner,tid)) {
		dprintf(0,"remove head of timer queue, belonging to tid = %d\n", tid.raw);
		alarm_timer* to_delete = timer_queue_head;
		timer_queue_head = timer_queue_head->next_alarm;

		if(timer_queue_head != NULL) {
			// restart with delay of new head
			timestamp_t delay = max(timer_queue_head->expiration_time - time_stamp(), 1);
			TIMER0_SET(MICROSECONDS_TO_TICKS(delay));
			TIMER0_START();
		}
		free(to_delete);
	}

	// remove all queue entries further down the queue belonging to tid
	alarm_timer** atp = (alarm_timer**) &timer_queue_head;
	for (; *atp != NULL; atp = &(*atp)->next_alarm) {
		dprintf(0,"walking timer queue, entry belongs to tid = 0x%X\n", (*atp)->owner);

		if(L4_IsThreadEqual((*atp)->next_alarm->owner,tid)) {
			dprintf(0,"remove entry from timer queue, belonging to tid = 0x%X\n", tid);
			alarm_timer* to_delete = (*atp)->next_alarm;
			(*atp)->next_alarm = (*atp)->next_alarm->next_alarm;
			free(to_delete);
		}

	}
}


/**
 * @return The microseconds since booting.
 */
timestamp_t time_stamp(void) {
	if(!driver_initialized)
		return CLOCK_R_UINT;

	timestamp_t current_low = (*(L4_Word_t*)OST_TS);
	return current_timestamp_high + TICKS_TO_MICROSECONDS(current_low);
}


/**
 * Stops our timer driver. This will turn off the Interrupts, unmap the
 * register fpage, stop the timre registers and wakeup all alarm_timers
 * currently in the queue with status CLOCK_R_CNCL.
 *
 * @return CLOCK_R_OK
 */
int stop_timer(void) {
	assert(driver_initialized);

	// disable interrupts
	L4_DeassociateInterrupt(timestamp_irq_tid);
	L4_DeassociateInterrupt(timer0_irq_tid);

	// unmap fpage
	L4_UnmapFpage(L4_Pager(), registers_fpage);

	TIMER0_STOP();

	alarm_timer* al = NULL;
	while( (al = timer_queue_pop()) != NULL ) {
		al->alarm_function(al->owner, CLOCK_R_CNCL);
		free(al);
	}

	driver_initialized = FALSE;
	return CLOCK_R_OK;
}


/**
 * Sleep system call handler (called by our syscall loop).
 * This will register a alarm_timer in our timer queue.
 *
 * @param tid Client Thread ID
 * @param msg_p IPC Message
 * @param ipc_memory Shared IPC memory region (not used)
 * @return 0 since we don't wan't to reply to client (yet)
 */
int sleep_timer(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr ipc_memory) {

	if(L4_UntypedWords(msg_p->tag) != 1)
		return IPC_SET_ERROR(CLOCK_R_FAIL);

	int sleep_time = L4_MsgWord(msg_p, 0);

	int ret = CLOCK_R_OK;
	if( (ret = register_timer((uint64_t)(sleep_time * 1000), tid)) != CLOCK_R_OK)
		return IPC_SET_ERROR(ret);

	return 0;
}


/**
 * Time stamp system call handler. Returns the value of timestamp.
 * We stick the 64byte value of time_stamp() into two 32bytes message
 * registers.
 *
 * @param tid Thread ID
 * @param msg_p IPC Message
 * @param ipc_memory Shared IPC memory (not used)
 * @return 1 since we wan't to return to the client immediately
 */
int send_timestamp(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr ipc_memory) {
	timestamp_t uptime = time_stamp();
	return set_ipc_reply(msg_p, 2, GET_LOW(uptime), GET_HIGH(uptime));
}


/*void test_the_clock() {
	assert(start_timer() == CLOCK_R_OK);
	assert(stop_timer() == CLOCK_R_OK);
	assert(register_timer(100ULL, L4_nilthread) == CLOCK_R_UINT);
}*/
