#include <assert.h>
#include <l4/thread.h>
#include <l4/misc.h>
#include <l4/space.h>
#include "clock.h"
#include "nslu2.h"

static timestamp timestamp = 0;
static alarm_timer* timer_queue_head = NULL;


#define GET_LOW(timestamp) ( ((L4_Word_t)(timestamp)) & 0xFFFFFFFF)
#define GET_HIGH(timestamp) ((timestamp) & 0xFFFFFFFF00000000)
#define OST_TS (NSLU2_OSTS_PHYS_BASE + 0)

void timer_queue_insert(alarm_timer* new_timer) {
	assert(a != NULL);
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
	alarm_timer* new_alarm = malloc( sizeof(alarm_timer) );
	new_alarm->expiration_time = time_stamp() + delay;
	new_alarm->alarm_function = &wakeup;
	new_alarm->owner = client;

	timer_queue_insert(new_alarm);

	return CLOCK_R_FAIL;
}


timestamp time_stamp(void) {

	timestamp current_low = TICKS_TO_MICROSECONDS( (*(L4_Word_t*)OST_TS) ); // TODO verfy correctness (no overflow while converting to microseconds?)
	if(GET_LOW(timestamp) > current_low)
		timestamp += ((timestamp)1) << 32;
	timestamp = GET_HIGH(timestamp) | current_low;

	return timestamp;
}


int stop_timer(void) {
	return CLOCK_R_FAIL;
}
