#include <assert.h>
#include <sos_shared.h>

#include "../l4.h"
#include "../libsos.h"
#include "../process.h"
#include "../io/io.h"
#include "swapper.h"

#define verbose 1

/** Amount of bytes read/written from/to swap file per call */
#define BATCH_SIZE 512

/** This variable multiplied by PAGESIZE gives the next free area in our swap file */
static int swap_entries = 0;

static L4_Bool_t is_dirty(page_queue_item* page) {
	return TRUE;
}

/*
static L4_Bool_t is_swapped(page_queue_item* page) {

}

*/

static L4_Bool_t is_referenced(page_queue_item* page) {
	L4_Fpage_t fpage = L4_FpageLog2(page->virtual_address, PAGESIZE_LOG2);
	L4_PhysDesc_t phys;

	if(L4_GetStatus(page->tid, &fpage, &phys) == FALSE) {
		dprintf(0, "Can't get status for page 0x%X (error:%d)\n", page->virtual_address, L4_ErrorCode());
	}

	return L4_WasReferenced(fpage);

}

static void dereference(page_queue_item* page) {

	if(L4_UnmapFpage(page->tid, L4_FpageLog2(page->virtual_address, PAGESIZE_LOG2)) == FALSE) {
		dprintf(0, "Can't unmap page at 0x%X (error:%d)\n", page->virtual_address, L4_ErrorCode());
	}

}


static page_queue_item* second_chance_select(struct pages_head* page_queue) {

	page_queue_item* page;

	// do a second chance search for a page over all currently active pages
    for(page = page_queue->tqh_first; page != NULL; page = page_queue->tqh_first) {
    	dprintf(0, "got page in page queue: thread:0x%X vaddr:0x%X\n", page->tid, page->virtual_address);

    	if(is_referenced(page)) {
    		dprintf(0, "page is referenced...\n");
    		TAILQ_REMOVE(page_queue, page, entries); // remove oldest page
    		dereference(page);
    		TAILQ_INSERT_TAIL(page_queue, page, entries); // insert at front
    	}
    	else {
    		// remove the page from the working set
    		return page;
    	}

    }

    return NULL; // no pages in queue (bad)
}

void swap_write_callback(uintptr_t token, int status, fattr_t *attr) {

	page_queue_item* page = (page_queue_item*) token;
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

				if(!is_dirty(page)) {
					// mark as swapped
					// free the frame
					// restart the thread who ran out of memory
				}
				else {
					// we have written the page while it was swapping out. this sux. restart.
					// swap_out();
				}
			} // else: not everything has been swapped yet...

		}
		break;

		case NFSERR_NOSPC:
			dprintf(0, "System ran out of memory _and_ swap space (this is bad).\n");
			assert(FALSE);
		break;

		default:
			dprintf(0, "%s: Bad status (%d) from callback.\n", __FUNCTION__, status);
			// TODO: how to handle this?
		break;
	}

}


void swap_read_callback(uintptr_t token, int status, fattr_t *attr, int bytes_read, char *data) {

	/*
	file_table_entry* f = (file_table_entry*) token;

	if(status == NFS_OK) {
		f->read_position += bytes_read;
		memcpy(f->client_buffer, data, bytes_read);
		send_ipc_reply(f->owner, CREATE_SYSCALL_NR(SOS_READ), 1, bytes_read);
	}
	else {
		dprintf(0, "%s: Bad status (%d) from callback.\n", __FUNCTION__, status);
		send_ipc_reply(f->owner, CREATE_SYSCALL_NR(SOS_READ), 1, -1);
	}*/

}



int swap_out(struct pages_head* page_queue) {
	page_queue_item* page = second_chance_select(page_queue);
	assert(page != NULL && !is_referenced(page));

	// decide where in the swap file our page will be
	page->swap_offset = (page->swap_offset >= 0) ? page->swap_offset : (swap_entries++ * PAGESIZE);

	dprintf(0, "second chance selected page: thread:0x%X vaddr:0x%X for swap\n", page->tid, page->virtual_address);

	if(is_dirty(page)) {
		dprintf(0, "selected page is dirty, need to write to swap space");
		page->to_swap = PAGESIZE;

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
				swap_fd->file->write_callback,
				(int)page
			);
		}

	}

	return 0;
}
