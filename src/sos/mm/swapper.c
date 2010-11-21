#include <assert.h>
#include <sos_shared.h>

#include "../l4.h"
#include "../libsos.h"
#include "swapper.h"

#define verbose 1

/*
static L4_Bool_t is_dirty(page_queue_item* page) {

}

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
    		TAILQ_REMOVE(page_queue, page, entries);
    		dereference(page);
    		TAILQ_INSERT_TAIL(page_queue, page, entries);
    	}
    	else {
    		return page;
    	}

    }

    return NULL; // no pages in queue (bad)
}

int swap_out(struct pages_head* page_queue) {
	page_queue_item* page = second_chance_select(page_queue);
	assert(page != NULL);

	dprintf(0, "second chance selected page: thread:0x%X vaddr:0x%X for swap\n", page->tid, page->virtual_address);
	return 0;
}
