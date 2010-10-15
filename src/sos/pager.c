/**
 * Page Table
 * =============
 * We're having a page size of 4096 bytes which requires us to use a
 * 12 bit offset. Since ARM uses 32bit addresses we can address with
 * the remaining 20 bits just about 2^20 frames (and since we only have about
 * 5k frames were good).
 *
 * Layout of our virtual address:
 * ------------------------------
 * First Level Table Index	(12 bits) 4096 Entries in 1st level table
 * 2nd Level Table Index 	( 8 bits)  256 Entries per table
 * Page Index				(12 bits)
 *							=========
 *							 32 bits
 *
 */

/** Small test to see if we have enough heap
void* first_level_table = malloc(4096*4);
assert(first_level_table != NULL);

for(int i=0; i<256; i++) {
	void* second_level_table = malloc(256*4);
	assert(second_level_table);
}
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <l4/types.h>

#include <l4/map.h>
#include <l4/misc.h>
#include <l4/space.h>
#include <l4/thread.h>

#include "pager.h"
#include "frames.h"
#include "libsos.h"
#include "syscalls.h"

#define verbose 2

#define PHYSICAL_MEMORY_END 0x2000000

#define FIRST_LEVEL_BITS 12
#define SECOND_LEVEL_BITS 8

#define FIRST_LEVEL_INDEX(addr)  ( ((addr) & 0xFFF00000) >> 20 )
#define SECOND_LEVEL_INDEX(addr) ( ((addr) & 0x000FF000) >> 12 )

/** List element used in the first level page table */
typedef struct page_entry {
	void* address;
} page_t;

static page_t* first_level_table = NULL;


/**
 * Performs a lookup in the first level page table.
 *
 * @param index where to look at
 * @return page table entry at position `index`
 */
static page_t* first_level_lookup(L4_Word_t index) {
	assert(index >= 0 && index < (1 << FIRST_LEVEL_BITS));

	return first_level_table+index;
}

/**
 * Performs a lookup in a second level page table.
 *
 * @param second_level_table which table to look in
 * @param index which table entry
 * @return table entry
 */
static page_t* second_level_lookup(page_t* second_level_table, L4_Word_t index) {
	assert(index >= 0 && index < (1 << SECOND_LEVEL_BITS));

	return second_level_table+index;
}

/**
 * Reserve space for 2nd level page table and point the corresponding
 * first level entry to it. In addition the allocated region is set to
 * zero.
 *
 * @param first_level_entry
 */
static void create_second_level_table(page_t* first_level_entry) {
	assert(first_level_entry->address == NULL); // TODO: do we want this?

	first_level_entry->address = malloc(1 << SECOND_LEVEL_BITS); // TODO: we need to care about freeing this later...
	assert(first_level_entry->address != NULL);

	memset(first_level_entry->address, 0, 1 << SECOND_LEVEL_BITS);
}


/** Initializes 1st level page table structure by allocating it on the heap. Initially all entries are set to 0. */
void pager_init() {
	first_level_table = malloc(1 << FIRST_LEVEL_BITS); // this is never freed but it's ok
	assert(first_level_table != NULL);

	memset(first_level_table, 0, 1 << FIRST_LEVEL_BITS);
}


/**
 * Method called by the SOS Server whenever a page fault occurs.
 *
 * @param tid the faulting thread id
 * @param msgP pointer to the page fault message
 */
void pager(L4_ThreadId_t tid, L4_Msg_t *msgP)
{
	assert( ((short) L4_Label(msgP->tag) >> 4) ==  L4_PAGEFAULT); // make sure pager is only called in page fault context

    // Get fault information
    L4_Word_t addr = L4_MsgWord(msgP, 0);
    L4_Word_t ip = L4_MsgWord(msgP, 1);
    L4_Word_t fault_reason = L4_Label(msgP->tag) & 0xF; // permissions are stored in lower 4 bit of label

	dprintf(1, "PAGEFAULT: pager(tid: %X,\n\t\t faulting_ip: 0x%X,\n\t\t faulting_addr: 0x%X,\n\t\t fault_reason: 0x%X)\n", tid, ip, addr, fault_reason);

	// For addresses below PHYSICAL_MEMORY_END we just do 1 to 1 mapping of addresses
	if(addr <= PHYSICAL_MEMORY_END) {
		// Construct fpage IPC message
		L4_Fpage_t targetFpage = L4_FpageLog2(addr, 12);
		L4_Set_Rights(&targetFpage, L4_FullyAccessible);

		// Assumes virtual - physical 1-1 mapping
		L4_PhysDesc_t phys = L4_PhysDesc(addr, L4_DefaultMemory);

		if ( !L4_MapFpage(tid, targetFpage, phys) ) {
			sos_print_error(L4_ErrorCode());
			printf(" Can't map page at %lx\n", addr);
		}
	}
	else { // perform a 2 level page table lookup

		page_t* first_entry = first_level_lookup(FIRST_LEVEL_INDEX(addr));
		if(first_entry->address == NULL) {
			create_second_level_table(first_entry); // set up new 2nd level page table
		}

		page_t* second_entry = second_level_lookup(first_entry->address, SECOND_LEVEL_INDEX(addr));
		if(second_entry->address == NULL) {

			second_entry->address = (void*) frame_alloc(); // 4096 byte aligned
			L4_Fpage_t targetFpage = L4_FpageLog2(addr, 12); // 1024 byte aligned
			L4_Set_Rights(&targetFpage, L4_ReadWriteOnly);

			L4_Word_t aligned_address = addr >> 10; // divide by 1024
			L4_Word_t frame_offset = aligned_address % 4;

			L4_Word_t mapping_address = (L4_Word_t) ((char*)second_entry->address)+(frame_offset*1024);

			L4_PhysDesc_t phys = L4_PhysDesc(mapping_address, L4_DefaultMemory);

			if ( !L4_MapFpage(tid, targetFpage, phys) ) {
				sos_print_error(L4_ErrorCode());
				printf(" Can't map page at %lx\n", addr);
			}
		}
		else {
			// what to do here? return address?
		}

	}

	// Generate Reply message
	L4_Set_MsgMsgTag(msgP, L4_Niltag);
	L4_MsgLoad(msgP);
}
