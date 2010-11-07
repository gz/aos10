#include <stdlib.h>
#include <assert.h>
#include <l4/thread.h>
#include <l4/misc.h>
#include <l4/space.h>
#include "clock.h"
#include "nslu2.h"

static timestamp_t current_timestamp = 0ULL;
static alarm_timer* timer_queue_head = NULL;


#define GET_LOW(timestamp) ( ((L4_Word_t)(timestamp)) & 0xFFFFFFFF)
#define GET_HIGH(timestamp) ((timestamp) & 0xFFFFFFFF00000000)
#define OST_TS (NSLU2_OSTS_PHYS_BASE + 0)

void timer_queue_insert(alarm_timer* new_timer) {
	assert(new_timer != NULL);

	if(timer_queue_head == NULL) {
		timer_queue_head = new_timer;
		return;
	}

	alarm_timer** atp = (alarm_timer**) timer_queue_head;
	for (; *atp != NULL; atp = &(*atp)->next_alarm) {
		 if (new_timer->expiration_time < (*atp)->expiration_time) break;
	}

	// insert new_timer in the middle or at the end of the list
	new_timer->next_alarm = *atp;
	*atp = new_timer; // set the previous next_alarm pointer to our new inserted timer
}

L4_Bool_t timer_queue_pop() {

	if(timer_queue_head != NULL) {
		alarm_timer* old_head = timer_queue_head;
		timer_queue_head = timer_queue_head->next_alarm;
		free(old_head);
		return 1;
	}
	else
		return 0;

}

int start_timer(void) {

	// Set up uncached memory mapping for registers
	L4_Fpage_t targetFpage = L4_FpageLog2(NSLU2_OSTS_PHYS_BASE, 12);
	L4_Set_Rights(&targetFpage, L4_FullyAccessible);

	L4_PhysDesc_t phys = L4_PhysDesc(NSLU2_OSTS_PHYS_BASE, L4_UncachedMemory);

	if(L4_MapFpage(L4_Pager(), targetFpage, phys)) {
		*((L4_Word_t*)OST_TS) = 0x00000000; // reset counter
		return CLOCK_R_OK;
	}
	else {
		return CLOCK_R_FAIL;
	}
}

static void wakeup(alarm_timer* a) {

}

/**
 *
 * @param delay
 * @param client
 * @return
 */
int register_timer(uint64_t delay, L4_ThreadId_t client) {
	alarm_timer* new_alarm = malloc( sizeof(alarm_timer) ); // free'd in timer_queue_remove_front
	new_alarm->alarm_function = &wakeup;
	new_alarm->owner = client;
	new_alarm->next_alarm = NULL;
	new_alarm->expiration_time = time_stamp() + delay;

	timer_queue_insert(new_alarm);
	if(timer_queue_head == new_alarm) {
		// TODO: reset timer counter
	}

	return CLOCK_R_OK;
}


timestamp_t time_stamp(void) {

	timestamp_t current_low = TICKS_TO_MICROSECONDS( (*(L4_Word_t*)OST_TS) ); // TODO verify correctness (no overflow while converting to microseconds?)
	if(GET_LOW(current_timestamp) > current_low)
		current_timestamp += ((timestamp_t)1) << 32;
	current_timestamp = GET_HIGH(current_timestamp) | current_low;

	return current_timestamp;
}


int stop_timer(void) {
	return CLOCK_R_FAIL;
}
