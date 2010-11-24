#ifndef SWAPPER_H_
#define SWAPPER_H_

#include <nfs.h>
#include "pager.h"
#include "../queue.h"

typedef struct pit {
	TAILQ_ENTRY(pit) entries;

	L4_ThreadId_t tid;
	L4_Word_t virtual_address;

	int swap_offset;
	int to_swap;
	L4_ThreadId_t initiator;
	//page_table_entry* table_entry;

} page_queue_item;

/** File descriptor Index for swap file in root process filetable */
#define SWAP_FD 1

// Return values for swap out
#define SWAPPING_PENDING -1
#define SWAPPING_COMPLETE -2

int swap_out(L4_ThreadId_t);
int swap_in(page_queue_item*);
void swap_write_callback(uintptr_t, int, fattr_t*);
void swap_read_callback(uintptr_t, int, fattr_t*, int, char*);

#endif /* SWAPPER_H_ */
