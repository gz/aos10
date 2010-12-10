/**
 * Swapping
 * =============
 * Swapping uses a file called swap in our filesystem to temporary
 * store pages once we run out of free frames. We implemented a
 * second chance page replacement algorithm to select which page
 * should be swapped out. This file provides 2 functions
 * swap_out and swap_in responsible for writing a page to the swap
 * file and reading it back in a frame. These functions are called
 * by the pager (see pager.c).
 *
 * Second Chance Implementation
 * ------------------------------
 * In addition to our software page table we use a queue containing
 * the current set of active pages in the system. second_chance_select
 * uses the queue to select a page for swapping out.
 *
 * Reference and Dirty Bits
 * ------------------------------
 * Since the ARM CPU does not provide any reference or dirty bit we
 * emulated them in software. So set/unset the reference bit
 * corresponds to mapping/unmapping the corresponding fpage.
 * The dirty bit is regarded as set as soon as a page is mapped with
 * write access into the pagetable.
 *
 * Pagetable Modifications
 * ------------------------------
 * We need know - when reading out page table entries - if a page is
 * currently swapped out or if it's dirty.
 * As soon as a page is swapped out the 1st bit in the page table is set
 * to 1 (Note: the lower 12 bits in the pagetable are never used anyways
 * by addresses). If this bit is one, the upper area of the entry stores
 * the offset were the page is located in the swap file.
 * A page is marked as dirty in the pagetable if the second bit is set.
 *
 * Swap File Layout
 * ------------------------------
 * The swap file contains the swapped out pages. Page are one after another
 * in the file (each page is 4096 bytes large). The entries of a swap file
 * are tracked using a bitfield. The maximum size of the swap file is
 * controlled by MAX_SWAP_ENTRIES. As a default value this is set to
 * 5000. So this means we can swap out MAX_SWAP_ENTRIES pages which limits
 * the maximum size of the swap file to PAGESIZE*MAX_SWAP_ENTRIES.
 *
 **/

#include <assert.h>
#include <string.h>
#include <sos_shared.h>

#include "../l4.h"
#include "../libsos.h"
#include "../process.h"
#include "../io/io.h"
#include "../datastructures/bitfield.h"
#include "swapper.h"
#include "frames.h"

#define verbose 3

/** Amount of bytes read/written from/to swap file per call */
#define BATCH_SIZE 512
/** Maximum number of entries the swap file can hold (note this should be a multiple of 8 for the bitfield) */
#define MAX_SWAP_ENTRIES 5000
static data_ptr swap_bitfield;

struct pages_head active_pages_head;
struct pages_head swapping_pages_head;


/**
 * Check if a page is dirty (has been written to).
 * @param page
 * @return 1 If page was written, 0 otherwise
 */
static L4_Bool_t is_dirty(page_queue_item* page) {
	assert(page != NULL);

	page_table_entry* pte = pager_table_lookup(page->tid, page->virtual_address);
	assert(pte != NULL);

	return (L4_Word_t)pte->address_ptr & 0x2;
}


/**
 * Checks if a page has been referenced. This works by querying
 * the L4 pagetable. If the page is mapped in there we say it's
 * referenced.
 * @param page
 * @return 1 If page was referenced, 0 otherwise
 */
static L4_Bool_t is_referenced(page_queue_item* page) {
	L4_Fpage_t fpage = L4_FpageLog2(page->virtual_address, PAGESIZE_LOG2);
	L4_PhysDesc_t phys;

	if(L4_GetStatus(page->tid, &fpage, &phys) == FALSE) {
		dprintf(0, "Can't get status for page 0x%X (error:%d)\n", page->virtual_address, L4_ErrorCode());
	}

	return L4_WasReferenced(fpage);

}


/**
 * Dereferences a page. This just unmaps it in the L4 page table.
 * @param page
 */
static void dereference(page_queue_item* page) {

	if(L4_UnmapFpage(page->tid, L4_FpageLog2(page->virtual_address, PAGESIZE_LOG2)) == FALSE) {
		dprintf(0, "Can't unmap page at 0x%X (error:%d)\n", page->virtual_address, L4_ErrorCode());
	}

	//L4_CacheFlushRange(page->tid, pager_table_lookup(page->tid, page->virtual_address)->address, pager_table_lookup(page->tid, page->virtual_address)->address+PAGESIZE);
}


/**
 * Second chance select implementation for choosing pages to
 * swap out. This will loop through the queue and find the
 * oldest unreferenced page.
 * Note: Since we use software emulated reference bits, in case
 * everything is mapped in the HW page table this degenerates
 * to pure FIFO.
 *
 * @param page_queue
 * @return oldest unreferenced page (removed from the queue)
 */
static page_queue_item* second_chance_select(struct pages_head* page_queue) {

	page_queue_item* page;

	// do a second chance search for a page over all currently active pages
    for(page = page_queue->tqh_first; page != NULL; page = page_queue->tqh_first) {

    	if(is_referenced(page)) {
    		TAILQ_REMOVE(page_queue, page, entries); // remove oldest page
    		dereference(page);
    		TAILQ_INSERT_TAIL(page_queue, page, entries); // insert at front
    	}
    	else {
    		// remove the page from the working set
    		TAILQ_REMOVE(page_queue, page, entries);
    		return page;
    	}

    }

    return NULL; // no pages in queue (bad)
}


/**
 * Marks a page as being swapped out and sets its swap location
 * in the pagetable.
 *
 * @param pte page table entry
 * @param swap_location offset in the swap file
 */
static void mark_swapped(page_table_entry* pte, int swap_location) {
	assert(swap_location % PAGESIZE == 0);
	assert(pte != NULL);

	pte->address = (swap_location | 0x1);
}


/**
 * Write callback for the NFS write function in case we swap out.
 * Note that because we cannot write 4096 bytes in one chunk
 * we split the writes up in 512 byte chunks. So this function
 * is called multiple times by the NFS library.
 *
 * @param token pointer to the page queue item we are swapping out
 */
static void swap_write_callback(uintptr_t token, int status, fattr_t *attr) {

	page_queue_item* page = (page_queue_item*) token;
	page_table_entry* pte = pager_table_lookup(page->tid, page->virtual_address);
	file_table_entry* swap_fd = get_process(root_thread_g)->filetable[SWAP_FD];

	switch (status) {
		case NFS_OK:
		{
			// update file attributes
			swap_fd->file->status.st_size = attr->size;
			swap_fd->file->status.st_atime = attr->atime.useconds / 1000;

			page->to_swap -= BATCH_SIZE;

			// swapping complete, can we now finally free the frame?
			if(page->to_swap == 0) {
				if(!is_referenced(page)) {

					dprintf(0, "page is swapped out\n");
					if(!page->process_deleted) {
						frame_free(CLEAR_LOWER_BITS(pte->address));
						mark_swapped(pte, page->swap_offset);

						// restart the thread who ran out of memory
						if(!L4_IsNilThread(page->initiator))
							send_ipc_reply(page->initiator, L4_PAGEFAULT, 0);
					}
					else {} // page was killed, nothing to do

					TAILQ_REMOVE(&swapping_pages_head, page, entries);
					free(page);

				}
				else {
					// page has been referenced inbetween swapping out
					// we need to restart the whole swap procedure
					TAILQ_REMOVE(&swapping_pages_head, page, entries);

					if(!L4_IsNilThread(page->initiator)) {
						send_ipc_reply(page->initiator, L4_PAGEFAULT, 0);
					}

					if(!page->process_deleted) {
						page->initiator = L4_nilthread;
						TAILQ_INSERT_TAIL(&active_pages_head, page, entries);
					}
					else {
						free(page);
					}

				}

			}

		}
		break;

		case NFSERR_NOSPC:
			dprintf(0, "System ran out of memory _and_ swap space (this is bad).\n");
			TAILQ_REMOVE(&swapping_pages_head, page, entries);
			free(page);
			assert(FALSE);
		break;

		default:
			dprintf(0, "%s: Bad NFS status (%d) from callback.\n", __FUNCTION__, status);
			TAILQ_REMOVE(&swapping_pages_head, page, entries);
			free(page);
			assert(FALSE);
			// We could probably try to restart swapping here but since it failed before
			// we don't see much point in this.
		break;
	}

}


/**
 * Read Callback for the NFS library if we're reading a page back into memory.
 * Again we split the NFS calls up and always read BATCH_SIZE bytes per call.
 * Note that once the page is competely swapped in it is inserted back into
 * the page queue which holds all the active pages at any given time.
 *
 * @param token pointer to the page item
 * @param bytes_read should always be BATCH_SIZE
 * @param data pointer to the data
 */
static void swap_read_callback(uintptr_t token, int status, fattr_t *attr, int bytes_read, char *data) {

	page_queue_item* page = (page_queue_item*) token;
	page_table_entry* pte = pager_table_lookup(page->tid, page->virtual_address);
	file_table_entry* swap_fd = get_process(root_thread_g)->filetable[SWAP_FD];

	if(page->process_deleted) {
		TAILQ_REMOVE(&swapping_pages_head, page, entries);
		free(page);
		return;
	}

	switch (status) {
		case NFS_OK:
		{
			assert(bytes_read == BATCH_SIZE);

			memcpy( ((char*)pte->address_ptr)+page->to_swap, data, bytes_read);
			page->to_swap += bytes_read;

			// swapping in complete
			if(page->to_swap == PAGESIZE) {
				// restart the thread because the page is in memory again
				TAILQ_REMOVE(&swapping_pages_head, page, entries);
	    		TAILQ_INSERT_TAIL(&active_pages_head, page, entries);
				send_ipc_reply(page->tid, L4_PAGEFAULT, 0);
			}
			else {
				// read next batch
				nfs_read(&swap_fd->file->nfs_handle, page->swap_offset+page->to_swap, BATCH_SIZE, &swap_read_callback, (int)page);
			}

		}
		break;

		default:
			dprintf(0, "%s: Bad NFS status (%d) from callback.\n", __FUNCTION__, status);
			assert(FALSE);
			// We could probably try to restart swapping here but since it failed before
			// we don't see much point in this.
		break;
	}

}


/**
 * Finds an empty place in the swap file.
 *
 * @return Start offset of free space in swap file or -1 if
 * swap is full.
 */
static int allocate_swap_entry(void) {

	for(int i=0; i<MAX_SWAP_ENTRIES; i++) {

		if(!bitfield_get(swap_bitfield, i)) {
			bitfield_set(swap_bitfield, i, 1);
			return i*PAGESIZE;
		}
	}

	return -1;
}


/**
 * Frees a swap entry at a given offset.
 * @param offset
 */
void swap_free(int offset) {
	assert(offset % PAGESIZE == 0);

	int bit_number = offset / PAGESIZE;
	assert(bit_number <= MAX_SWAP_ENTRIES);
	assert(bitfield_get(swap_bitfield, bit_number));

	bitfield_set(swap_bitfield, bit_number, 0);
}


/**
 * Returns the bitfield value of a given
 * swap offset.
 * @param offset
 * @return true or false
 */
L4_Bool_t swap_get(int offset) {
	assert(offset % PAGESIZE == 0);

	int bit_number = offset / PAGESIZE;
	assert(bit_number <= MAX_SWAP_ENTRIES);

	return bitfield_get(swap_bitfield, bit_number);
}


/**
 * Initializes datastructures required for swapping.
 * This is called by pager_init.
 */
void swap_init() {
	TAILQ_INIT(&active_pages_head);
	TAILQ_INIT(&swapping_pages_head);

	// initialize the bitfield for the swap file
	swap_bitfield = malloc(MAX_SWAP_ENTRIES / 8);
	for(int i=0; i < MAX_SWAP_ENTRIES; i++) {
		bitfield_set(swap_bitfield, i, 0);
	}
}


/**
 * This function will select a page (based on second chance)
 * and swap it out to file system. However if the selected page
 * was not dirty then we just need to mark it swapped again and
 * can return immediatly.
 * We stop the initiator thread as long as we're swapping
 * data out to the file system. The thread is restarted in the
 * write callback function (above).
 *
 * @param initiator ID of the thread who caused the swapping to happen
 * @return	SWAPPING_PENDING in case we need to write the page to the disk
 * 			SWAPPING_COMPLETE in case the page was not dirty
 * 			OUT_OF_SWAP_SPACE if our swap space is already full
 * 			NO_PAGE_AVAILABLE if second_chance_select did not find a page
 */
int swap_out(L4_ThreadId_t initiator) {
	page_queue_item* page = second_chance_select(&active_pages_head);
	if(page == NULL) {
		return NO_PAGE_AVAILABLE;
	}
	assert(!is_referenced(page));

	// decide where in the swap file our page will be
	if(page->swap_offset < 0 && (page->swap_offset = allocate_swap_entry()) < 0)
		return OUT_OF_SWAP_SPACE;

	dprintf(3, "swap_out: Second chance selected page: thread:0x%X vaddr:0x%X swap_offset:0x%X\n", page->tid, page->virtual_address, page->swap_offset);

	if(is_dirty(page)) {
		dprintf(1, "Selected page is dirty, need to write to swap space\n");

		page->to_swap = PAGESIZE;
		page->initiator = initiator;
		TAILQ_INSERT_TAIL(&swapping_pages_head, page, entries);

		file_table_entry* swap_fd = get_process(root_thread_g)->filetable[SWAP_FD];
		assert(swap_fd != NULL);

		data_ptr physical_page = pager_physical_lookup(page->tid, page->virtual_address);
		assert(physical_page != NULL);

		// write page in swap file
		assert(PAGESIZE % BATCH_SIZE == 0);
		for(int write_offset=0; write_offset < page->to_swap; write_offset += BATCH_SIZE) {
			nfs_write(
				&swap_fd->file->nfs_handle,
				page->swap_offset + write_offset,
				BATCH_SIZE,
				physical_page + write_offset,
				&swap_write_callback,
				(int)page
			);
		}

		return SWAPPING_PENDING;
	}
	else {
		assert(page->swap_offset >= 0);
		page_table_entry* pte = pager_table_lookup(page->tid, page->virtual_address);
		frame_free(CLEAR_LOWER_BITS(pte->address));
		mark_swapped(pte, page->swap_offset);
		free(page);

		return SWAPPING_COMPLETE;
	}

}


/**
 * Called by the pager. Reads a given page back into memory.
 *
 * @param page the page we wan't to read back into memory
 * @return SWAPPING_PENDING
 */
int swap_in(page_queue_item* page) {
	assert(page != NULL);
	file_table_entry* swap_fd = get_process(root_thread_g)->filetable[SWAP_FD];
	assert(swap_fd != NULL);

	page->to_swap = 0; // to keep track of how many bytes are read
	TAILQ_INSERT_TAIL(&swapping_pages_head, page, entries);

	nfs_read(&swap_fd->file->nfs_handle, page->swap_offset+page->to_swap, BATCH_SIZE, &swap_read_callback, (int)page);

	return SWAPPING_PENDING;
}
