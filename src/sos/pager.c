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

#include <stdio.h>
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

/** List element used in the first level page table */
typedef struct page_entry {
	void* address;
} page_t;


/** Initializes 1st level page table structure by allocating it on the heap. */
void pager_init() {
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
	else {
		// wohoo my fancy pager here
	}

	// Generate Reply message
	L4_Set_MsgMsgTag(msgP, L4_Niltag);
	L4_MsgLoad(msgP);
}
