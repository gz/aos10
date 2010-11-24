/**
 * Page Table
 * =============
 * We're having a page size of 4096 bytes which requires us to use a
 * 12 bit offset. Since ARM uses 32 bit addresses we can address with
 * the remaining 20 bits just about 2^20 frames (and since we only have about
 * 5k frames were good).
 * One page table entry in both 1st and 2nd tables is always 4 byte (type: `page_t`).
 * Because of our virtual address layout (see below) the page table structure
 * requires about (2^12 * 4) + (2^12 * (2^8 * 4)) bytes = 4 megabytes of space
 * if our virtual address space would be filled out completely.
 *
 * The pager_unmap_all is used for testing the page table. It walks through it,
 * reconstructs all the fpages from the table entries and unmaps them. This can
 * be called by a given thread through the syscall SOS_UNMAP_ALL.
 *
 * Layout of our virtual address:
 * ------------------------------
 * First Level Table Index	(12 bits) 4096 Entries in 1st level table
 * 2nd Level Table Index 	( 8 bits)  256 Entries per table
 * Page Index				(12 bits) 4096 Bytes offset to address within page
 *							=========
 *							 32 bits
 *
 * Layout of our virtual address space:
 * ------------------------------------
 *      [[  TEXT  |  DATA  |  HEAP  |  NoAccess | IPC Memory |  NoAccess  |  STACK  |  NoAccess  ]]
 * 0x02000000         0x40000000            0x60000000               0xC0000000
 *
 * Limitations:
 * ------------------------------------
 * The boundaries for code heap, stack area are static. We implemented have
 * the following limits for the heap/stack size:
 *
 * Max Heap Size:     4 MB
 * Max Stack Size:    1 MB
 * Max Binary Size: 992 MB [Not used now, will get smaller later]
 *
 *
 **/

/** Small test to see if we have enough heap
void* first_level_table = malloc(4096*4);
assert(first_level_table != NULL);

for(int i=0; i<4096; i++) {
	void* second_level_table = malloc(256*4);
	assert(second_level_table != NULL);
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
#include <l4/cache.h>

#include "pager.h"
#include "swapper.h"
#include "frames.h"
#include "../libsos.h"

#define verbose 3

// Return codes for virtual_mapping()
#define MAPPING_FAILED 0
#define MAPPING_SUCCESS 1
#define OUT_OF_FRAMES 2
#define PAGE_NOT_AVAILABLE 3

// Virtual address space layout constants
#define ONE_MEGABYTE (1024*1024)
#define VIRTUAL_START 0x2000000

#define STACK_TOP 0xC0000000
#define STACK_END (STACK_TOP - ONE_MEGABYTE)

#define IPC_START 0x60000000
#define IPC_END (IPC_START + 4096)

#define HEAP_START 0x40000000
#define HEAP_END (HEAP_START + (4 * ONE_MEGABYTE))

// Page table manipulation macros and constants
#define FIRST_LEVEL_BITS 12
#define FIRST_LEVEL_ENTRIES (1 << FIRST_LEVEL_BITS)
#define SECOND_LEVEL_BITS 8
#define SECOND_LEVEL_ENTRIES (1 << SECOND_LEVEL_BITS)

#define FIRST_LEVEL_INDEX(addr)  ( (((addr) & 0xFFF00000) >> 20) & 0xFFF )
#define SECOND_LEVEL_INDEX(addr) (  ((addr) & 0x000FF000) >> 12 )
#define CREATE_ADDRESS(first, second) ( ((first) << 20) | ((second) << 12) )

#define IS_SWAPPED(addr)  ((addr) & 0x1)

// Global datastructures for pager
static page_table_entry* first_level_table = NULL;
struct pages_head active_pages_head;


/**
 * Gets access rights for a given thread at a certain memory location.
 *
 * @param tid ID of the thread
 * @param addr Location we wan't to know the access rights
 * @return 3 bits set/unset depending on read write exec access (refer to l4/types.h)
 */
static L4_Word_t get_access_rights(L4_ThreadId_t tid, L4_Word_t addr) {

	// Null pointer exception
	if(addr < 4096) {
		return L4_NoAccess;
	}

	// User space physical memory permission
	if(addr < VIRTUAL_START)
		return L4_FullyAccessible; // TODO: when we have binary in virtual memory we can set this to L4_NoAccess

	// Heap permissions
	if(addr >= HEAP_START && addr < HEAP_END)
		return L4_ReadWriteOnly;

	// IPC permissions
	if(addr >= IPC_START && addr < IPC_END)
		return L4_ReadWriteOnly;

	// Stack permissions
	if(addr > STACK_END  && addr <= STACK_TOP)
		return L4_ReadWriteOnly;

	return L4_NoAccess;
}


/**
 * Checks if a given thread has the access he requests for a certain memory region.
 *
 * @param tid ID of the requesting thread
 * @param addr Memory location he's requesting
 * @param tried_access type of access he wants for the given location
 * @return 1 if he's allowed to access, 0 otherwise
 */
inline static L4_Bool_t is_access_granted(L4_ThreadId_t tid, L4_Word_t addr, L4_Word_t tried_access) {
	return (tried_access & get_access_rights(tid, addr)) == tried_access;
}


/**
 * Performs a lookup in the first level page table.
 *
 * @param index where to look at
 * @return page table entry at position `index`
 */
static page_table_entry* first_level_lookup(L4_Word_t index) {
	assert(index >= 0 && index < FIRST_LEVEL_ENTRIES);

	return first_level_table+index;
}


/**
 * Performs a lookup in a second level page table.
 *
 * @param second_level_table which table to look in
 * @param index which table entry
 * @return table entry
 */
static page_table_entry* second_level_lookup(page_table_entry* second_level_table, L4_Word_t index) {
	assert(index >= 0 && index < SECOND_LEVEL_ENTRIES);
	return second_level_table+index;
}


/**
 * Reserve space for 2nd level page table and point the corresponding
 * first level entry to it. In addition the allocated region is set to
 * zero.
 *
 * @param first_level_entry
 */
static void create_second_level_table(page_table_entry* first_level_entry) {
	assert(first_level_entry->address == NULL);

	first_level_entry->address = malloc(SECOND_LEVEL_ENTRIES * sizeof(page_table_entry));
	assert(first_level_entry->address != NULL);

	memset(first_level_entry->address, 0, SECOND_LEVEL_ENTRIES * sizeof(page_table_entry));
}


/**
 * Marks a page as dirty in the page table.
 * @param pte page table entry pointer
 */
static void mark_dirty(page_table_entry* pte) {
	assert(pte != NULL);
	assert(!IS_SWAPPED((L4_Word_t)pte->address)); // marking swapped page dirty would not make sense

	L4_Word_t current = (L4_Word_t) pte->address;
	pte->address = (void*) (current | 0x2);
}


/**
 * Allocates a page_queue_item and initializes it with the arguments given.
 * @return pointer to the page_queue_item
 */
static page_queue_item* create_page_queue_item(L4_ThreadId_t tid, L4_Word_t addr, int swap_offset) {
	page_queue_item* p = malloc(sizeof(page_queue_item)); // freed by swap out or (TODO) process delete
	assert(p != NULL);

	p->tid = tid;
	p->virtual_address = addr;
	p->swap_offset = swap_offset;

	return p;
}


/**
 * Initializes 1st level page table structure by allocating it on the heap.
 * Initially all entries are set to 0.
 */
void pager_init() {
	first_level_table = malloc(FIRST_LEVEL_ENTRIES * sizeof(page_table_entry)); // this is never freed but it's ok (for now) TODO
	assert(first_level_table != NULL);

	memset(first_level_table, 0, FIRST_LEVEL_ENTRIES * sizeof(page_table_entry));


	TAILQ_INIT(&active_pages_head);
}


/**
 * Performs a one to one mapping between virtual and physical address. This is used to address the RAM directly.
 *
 * @param tid ID of the thread to map for
 * @param addr memory location to map
 * @return 	MAPPING_FAILED iff Mapping failed
 * 			MAPPING_SUCCESS iff Mappfing success
 */
static L4_Bool_t one_to_one_mapping(L4_ThreadId_t tid, L4_Word_t addr, L4_Word_t requested_access) {

	// Construct fpage IPC message
	L4_Fpage_t targetFpage = L4_FpageLog2(addr, 12);
	L4_Set_Rights(&targetFpage, L4_FullyAccessible);

	// Assumes virtual - physical 1-1 mapping
	L4_PhysDesc_t phys = L4_PhysDesc(addr, L4_DefaultMemory);

	return L4_MapFpage(tid, targetFpage, phys);
}


/**
 * Allocates a new frame for a given thread.
 * In case we run out of free frame this method initiates
 * the swapping.
 * @param for_thread Thread ID which requested a frame
 * @return NULL in case no frame can be returned yet
 * valid pointer to a frame otherwise.
 */
static void* allocate_new_frame(L4_ThreadId_t for_thread) {

	void* new_frame = NULL;
	if((new_frame = (void*)frame_alloc()) == NULL) {

		switch(swap_out(for_thread)) {

			case SWAPPING_COMPLETE:
				// non dirty pages can be swapped and free'd
				// without requiring any IO
				new_frame = (void*)frame_alloc();
				assert(new_frame != NULL);
			break;

			default:
				// We need to wait until the page
				// is written to the disk
			break;

		}

	}

	return new_frame;
}


/**
 * Does a 2 Level Pagetable Lookup to map the virtual address space into
 * physical. In addition swapping in and/or swapping out is initialized
 * if we run out of physical memory or a requested page is currently
 * swapped out. Newly mapped pages are inserted into the active
 * pages queue (used by swapper.c).
 *
 * @param tid ID of thread to map for
 * @param addr memory area to map (aligned to multiple of PAGESIZE)
 * @return	MAPPING_FAILED iff Mapping failed
 *			MAPPING_SUCCESS iff Mapping success
 *			OUT_OF_FRAMES iff no more free frames (swapping started)
 *			PAGE_NOT_AVAILABLE iff the page needs to be swapped in
 */
static int virtual_mapping(L4_ThreadId_t tid, L4_Word_t addr, L4_Word_t requested_access) {
	assert(is_access_granted(tid, addr, requested_access));

	// align address to be multiple of pagesize
	addr = ((addr >> PAGESIZE_LOG2) << PAGESIZE_LOG2);

	page_table_entry* first_entry = first_level_lookup(FIRST_LEVEL_INDEX(addr));
	if(first_entry->address == NULL) {
		dprintf(3, "No 2nd Level table exists here. Create one.\n");
		create_second_level_table(first_entry); // set up new 2nd level page table
	}

	page_table_entry* second_entry = second_level_lookup(first_entry->address, SECOND_LEVEL_INDEX(addr));

	// Page referenced for the first time
	if(second_entry->address == NULL) {

		if( (second_entry->address = allocate_new_frame(tid)) == NULL ) {
			return OUT_OF_FRAMES;
		}

		// insert page into queue of active pages
		page_queue_item* p = create_page_queue_item(tid, addr, -1);
		TAILQ_INSERT_TAIL(&active_pages_head, p, entries);
		// new allocated pages are always dirty
		mark_dirty(second_entry);

		dprintf(2, "New allocated physical frame: %X\n", (int) second_entry->address);
	}

	// Page is currently swapped out
	else if(IS_SWAPPED((L4_Word_t)second_entry->address)) {
		L4_Word_t swap_offset = CLEAR_LOWER_BITS((L4_Word_t)second_entry->address);

		void* new_frame = NULL;
		if( (new_frame = allocate_new_frame(tid)) == NULL ) {
			return OUT_OF_FRAMES;
		}

		second_entry->address = new_frame;
		page_queue_item* p = create_page_queue_item(tid, addr, swap_offset);
		swap_in(p);
		return PAGE_NOT_AVAILABLE;
	}

	if(requested_access & L4_Writable)
		mark_dirty(second_entry);

	// else page just isn't mapped in hardware
	L4_Fpage_t targetFpage = L4_FpageLog2(addr, PAGESIZE_LOG2);
	L4_Set_Rights(&targetFpage, requested_access);
	L4_PhysDesc_t phys = L4_PhysDesc(CLEAR_LOWER_BITS((L4_Word_t)second_entry->address), L4_DefaultMemory);

	dprintf(1, "Trying to map virtual address %X with physical %X\n", addr, CLEAR_LOWER_BITS((L4_Word_t)second_entry->address));
	return L4_MapFpage(tid, targetFpage, phys);
}


/**
 * Method called by the SOS Server whenever a page fault occurs.
 *
 * @param tid the faulting thread id
 * @param msgP pointer to the page fault message
 */
int pager(L4_ThreadId_t tid, L4_Msg_t *msgP)
{
	assert( ((short) L4_Label(msgP->tag) >> 4) ==  L4_PAGEFAULT); // make sure pager is only called in page fault context

    // Get fault information
    L4_Word_t addr = L4_MsgWord(msgP, 0);
    L4_Word_t ip = L4_MsgWord(msgP, 1);
    L4_Word_t fault_reason = L4_Label(msgP->tag) & 0xF; // permissions are stored in lower 4 bit of label

	dprintf(2, "PAGEFAULT: pager(tid: %X,\n\t\t faulting_ip: 0x%X,\n\t\t faulting_addr: 0x%X,\n\t\t fault_reason: 0x%X)\n", tid.global.X.thread_no, ip, addr, fault_reason);

	if(!is_access_granted(tid, addr, fault_reason)) {
		dprintf(0, "Thread:%X is trying to access memory location (0x%X) for rwx:0x%X\nbut it only has rights 0x%X in this region.\n", tid, addr, fault_reason, get_access_rights(tid, addr));
		L4_KDB_Enter("panic");
	}

	if(addr < VIRTUAL_START) {

		// For addresses below VIRTUAL_START we just do 1 to 1 mapping of addresses
		if (!one_to_one_mapping(tid, addr, fault_reason)) {
			sos_print_error(L4_ErrorCode());
			dprintf(0, "Can't map page at %lx\n", addr);
		}

	}
	else {

		// perform a 2 level page table lookup
		int ret = virtual_mapping(tid, addr, fault_reason);
		switch(ret) {
			case MAPPING_FAILED:
				sos_print_error(L4_ErrorCode());
				dprintf(0, "Can't map page at %lx. Either MapFpage failed or we're out of memory.\n", addr);
			break;

			case OUT_OF_FRAMES:
				// we're swapping and we have stopped the thread so don't send a reply (yet)
				// the thread will be started again once swapping is completed
				// (see swap_write_callback)
				return 0;
			break;

			case PAGE_NOT_AVAILABLE:
				// we need to swap-in the requested page first
				// since this takes some time we stopped the thread (and restart it when it's done)
				// and don't return a IPC message
				return 0;
			break;
		}

	}

	// Generate Reply message
	L4_Set_MsgMsgTag(msgP, L4_Niltag);
	L4_MsgLoad(msgP);
	return 1; // send the reply
}


/**
 * System call handler
 * This function unmaps all fpages for a given thread mapped to physical
 * memory by the pager. And flushes the CPU Cache.
 */
int pager_unmap_all(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {

	// TODO: select first level table based on tid

	for(int i=0; i<FIRST_LEVEL_ENTRIES; i++) {

		void* second_level_table = first_level_lookup(i)->address;
		if(second_level_table != NULL) {

			for(int j=0; j<SECOND_LEVEL_ENTRIES; j++) {
				if(second_level_lookup(second_level_table, j)->address != NULL) {

					L4_Word_t addr = CREATE_ADDRESS(i,j);
					dprintf(3, "Unmap for id:%X at 1st:%d 2nd:%d which corresponds to address %X\n", tid, i, j, addr);

					if(L4_UnmapFpage(tid, L4_FpageLog2(addr, PAGESIZE_LOG2)) == 0) {
						sos_print_error(L4_ErrorCode());
						dprintf(0, "Can't unmap page at %lx\n", addr);
					} // else success
				} // else no 2nd level entry here
			}

		} // else no 1st level entry here

	}

	// make sure to flush the cache otherwise there might still be some mappings in the cache
	L4_CacheFlushAll();

	return 0; // send no reply
}


/**
 * Frees the allocated space for the page table for a given thread.
 * This should be called after a thread is finished.
 *
 * @param tid thread id
 */
void pager_free_all(L4_ThreadId_t tid) {

	// TODO: select 1st level table based on tid

	for(int i=0; i<FIRST_LEVEL_ENTRIES; i++) {

		void* second_level_table = first_level_lookup(i)->address;
		if(second_level_table != NULL) {
			free(second_level_table);
		}

	}
	//TODO: free(first_level_table);

	L4_CacheFlushAll();
}


/**
 * Returns the corresponding physical address (or swap offset)
 * of a given virtual address.
 * @param tid Thread ID of the process
 * @param addr virtual address we want to look up
 */
void* pager_physical_lookup(L4_ThreadId_t tid, L4_Word_t addr) {

	page_table_entry* first_entry = first_level_lookup(FIRST_LEVEL_INDEX(addr));
	if(first_entry->address == NULL)
		return NULL;

	page_table_entry* second_entry = second_level_lookup(first_entry->address, SECOND_LEVEL_INDEX(addr));

	return (void*)CLEAR_LOWER_BITS((L4_Word_t)second_entry->address);
}


/**
 * Returns the page table entry for a given virtual address.
 * @param tid thread ID defining which pagetable
 * @param addr virtual address
 * @return pointer to the page table entry
 */
page_table_entry* pager_table_lookup(L4_ThreadId_t tid, L4_Word_t addr) {
	page_table_entry* first_entry = first_level_lookup(FIRST_LEVEL_INDEX(addr));
	if(first_entry->address == NULL)
		return NULL;

	return second_level_lookup(first_entry->address, SECOND_LEVEL_INDEX(addr));
}
