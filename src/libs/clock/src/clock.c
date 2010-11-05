#include <l4/thread.h>
#include <l4/misc.h>
#include <l4/space.h>
#include "clock.h"
#include "nslu2.h"

static timestamp_t timestamp = 0;

#define GET_LOW(timestamp) ( ((L4_Word_t)(timestamp)) & 0xFFFFFFFF)
#define GET_HIGH(timestamp) ((timestamp) & 0xFFFFFFFF00000000)

#define OST_TS (NSLU2_OSTS_PHYS_BASE + 0)

int start_timer(void) {

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

/**
 *
 * @param delay
 * @param client
 * @return
 */
int register_timer(uint64_t delay, L4_ThreadId_t client) {
	return CLOCK_R_FAIL;
}


timestamp_t time_stamp(void) {

	L4_Word_t current_low = (*(L4_Word_t*)OST_TS);
	if(GET_LOW(timestamp) > current_low)
		timestamp += ((timestamp_t)1) << 32;
	timestamp = GET_HIGH(timestamp) | current_low;

	return timestamp;
}


int stop_timer(void) {
	return CLOCK_R_FAIL;
}
