#ifndef SWAPPER_H_
#define SWAPPER_H_

#include <nfs.h>
#include "pager.h"
#include "../queue.h"

/** Information tracked for all active pages */
typedef struct pit {
	TAILQ_ENTRY(pit) entries;

	L4_ThreadId_t tid;			/**< Owner of the page */
	L4_Word_t virtual_address;	/**< Virtual address which accesses the page */

	int swap_offset;			/**< Where in the swap file this page is located, -1 if currently not in swap */
	int to_swap;				/**< Used to keep track of how many bytes have been swapped out/in already */
	L4_ThreadId_t initiator;	/**< Used by swap out (this thread is restarted after we finished swapping out) */
} page_queue_item;

TAILQ_HEAD(pages_head, pit);
extern struct pages_head active_pages_head;

/** File descriptor Index for swap file in root process filetable */
#define SWAP_FD 1

// Return values for swap out
#define SWAPPING_PENDING -1
#define SWAPPING_COMPLETE -2
#define OUT_OF_SWAP_SPACE -3
#define NO_PAGE_AVAILABLE -4

void swap_free(int);
void swap_init(void);
int swap_out(L4_ThreadId_t);
int swap_in(page_queue_item*);

#endif /* SWAPPER_H_ */
