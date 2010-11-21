#ifndef SWAPPER_H_
#define SWAPPER_H_

#include "pager.h"
#include "../queue.h"

typedef struct pit {
	TAILQ_ENTRY(pit) entries;

	L4_ThreadId_t tid;
	L4_Word_t virtual_address;

	//page_table_entry* table_entry;

} page_queue_item;

int swap_out(struct pages_head*);

#endif /* SWAPPER_H_ */
