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

static timestamp_t current_timestamp_high = 0ULL;
static alarm_timer* timer_queue_head = NULL;
static L4_Bool_t driver_initialized = FALSE;

static L4_ThreadId_t timestamp_irq_tid;
static L4_ThreadId_t timer0_irq_tid;
static L4_Fpage_t registers_fpage;

// Timer Register Addresses
#define OST_TS (NSLU2_OSTS_PHYS_BASE + 0x0)
#define OST_TIM0 (NSLU2_OSTS_PHYS_BASE + 0x4)
#define OST_TIM0_RL (NSLU2_OSTS_PHYS_BASE + 0x8)
#define OST_STATUS (NSLU2_OSTS_PHYS_BASE + 0x20)

// Macros to control Timer 0
//#define TIMER0_SET(count)	((*(L4_Word_t*)OST_TIM0_RL)  = ((count) & ~0x3) | ((*(L4_Word_t*)OST_TIM0_RL) & 0x3) )
//#define TIMER0_START()		((*(L4_Word_t*)OST_TIM0_RL) |=  1)
#define TIMER0_SET(count)	((*(L4_Word_t*)OST_TIM0_RL)  = ((count) & ~0x3) | 0x2 )
#define TIMER0_START()		((*(L4_Word_t*)OST_TIM0_RL) |= 0x1)
#define TIMER0_STOP()		((*(L4_Word_t*)OST_TIM0_RL)  = (*(L4_Word_t*)OST_TIM0_RL) & ~0x1)
#define TIMER0_ONE_SHOT(v) 	((*(L4_Word_t*)OST_TIM0_RL) |= (v) << 1)

// Macros to construct 64bit timestamp from 32bit timer
#define GET_LOW(timestamp) ( ((L4_Word_t)(timestamp)) & 0xFFFFFFFF)
#define GET_HIGH(timestamp) ((timestamp) & 0xFFFFFFFF00000000)


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

alarm_timer* timer_queue_pop() {

	if(timer_queue_head != NULL) {
		alarm_timer* old_head = timer_queue_head;
		timer_queue_head = timer_queue_head->next_alarm;
		return old_head;
	}
	else
		return NULL;

}

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

int timer_overflow_irq(L4_ThreadId_t tid, L4_Msg_t* msg_p) {
	assert( ((*(L4_Word_t*)OST_STATUS) >> 2) & 0x1); // this must only be called during interrupt

	current_timestamp_high += TICKS_TO_MICROSECONDS(0xFFFFFFFF);
	(*(L4_Word_t*)OST_STATUS) |= (0x1 << 2); // clear the timestamp timer interrupt bit

	return set_ipc_reply(msg_p, 0); // return to interrupt thread with 0 msg
}


int timer0_irq(L4_ThreadId_t tid, L4_Msg_t* msg_p) {
	assert( ((*(L4_Word_t*)OST_STATUS)) & 0x1); // this must only be called during interrupt
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

	(*(L4_Word_t*)OST_STATUS) |= 0x1; // clear the timestamp timer interrupt bit
	return set_ipc_reply(msg_p, 0); // return to interrupt thread with 0 msg
}



static void wakeup(L4_ThreadId_t tid, int status) {
	dprintf(1, "Alarm Timer from:0x%X is ringing with status:%d\n", tid, status);
	send_ipc_reply(tid, SOS_SLEEP, 1, status);
}

/**
 *
 * @param delay
 * @param client
 * @return
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


timestamp_t time_stamp(void) {
	if(!driver_initialized)
		return CLOCK_R_UINT;

	timestamp_t current_low = (*(L4_Word_t*)OST_TS);
	return current_timestamp_high + TICKS_TO_MICROSECONDS(current_low);
}


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


int sleep_timer(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr ipc_memory) {

	if(L4_UntypedWords(msg_p->tag) != 1)
		return IPC_SET_ERROR(CLOCK_R_FAIL);

	int sleep_time = L4_MsgWord(msg_p, 0);

	int ret = CLOCK_R_OK;
	if( (ret = register_timer((uint64_t)(sleep_time * 1000), tid)) != CLOCK_R_OK)
		return IPC_SET_ERROR(ret);

	return 0;
}


int send_timestamp(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr ipc_memory) {
	timestamp_t uptime = time_stamp();
	return set_ipc_reply(msg_p, 2, GET_LOW(uptime), GET_HIGH(uptime));
}

/*void test_the_clock() {
	assert(start_timer() == CLOCK_R_OK);
	assert(stop_timer() == CLOCK_R_OK);
	assert(register_timer(100ULL, L4_nilthread) == CLOCK_R_UINT);
}*/
