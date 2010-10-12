/**
 * Page Table
 * =============
 * We're having a Pagesize of 4096 bytes which requires us to use a
 * 12 bit offset. Since ARM uses 32bit addresses we can address with
 * the remaining 20 bits just about 2^20 frames (And since we only have about
 * 5k frames were good).
 *
 * Layout of our virtual address:
 * ------------------------------
 * First Level Table Index	(12 bits) 4096 Entries
 * 2nd Level Table Index 	( 8 bits)  256 Entries
 * Page Index				(12 bits)
 *							=========
 *							 32 bits
 *
 */

//
// Pager is called from the syscall loop whenever a page fault occurs. The
// current implementation simply maps whichever pages are asked for.
//
#include <stdio.h>
#include <assert.h>

#include <l4/types.h>

#include <l4/map.h>
#include <l4/misc.h>
#include <l4/space.h>
#include <l4/thread.h>

#include "pager.h"
#include "libsos.h"
#include "syscalls.h"

#define verbose 2

typedef struct page_entry {
	void* address;
} page;

void pager_init() {

}

void pager(L4_ThreadId_t tid, L4_Msg_t *msgP)
{
	assert( ((short) L4_Label(msgP->tag) >> 4) ==  L4_PAGEFAULT); // make sure pager is only called in pagefault context

    // Get the fault info
    L4_Word_t addr = L4_MsgWord(msgP, 0);
    L4_Word_t ip = L4_MsgWord(msgP, 1);
    L4_Word_t fault_reason = L4_Label(msgP->tag) & 0xF; // permissions are stored in lower 4 bit of label

	dprintf(1, "PAGEFAULT: pager(tid: %X,\n\t\t faulting_ip: 0x%X,\n\t\t faulting_addr: 0x%X,\n\t\t fault_reason: 0x%X)\n", tid, ip, addr, fault_reason);

	if(addr <= 0x2000000) {
		// Construct fpage IPC message
		L4_Fpage_t targetFpage = L4_FpageLog2(addr, 12);
		L4_Set_Rights(&targetFpage, L4_FullyAccessible);

		// Assumes virt - phys 1-1 mapping
		L4_PhysDesc_t phys = L4_PhysDesc(addr, L4_DefaultMemory);

		if ( !L4_MapFpage(tid, targetFpage, phys) ) {
		sos_print_error(L4_ErrorCode());
		printf(" Can't map page at %lx\n", addr);
		}

		// Generate Reply message
		L4_Set_MsgMsgTag(msgP, L4_Niltag);
		L4_MsgLoad(msgP);
	}
	else {
		// wohoo my fancy pager here
	}
}
