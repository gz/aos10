#ifndef PAGER_H_
#define PAGER_H_

#include <l4/types.h>
#include <l4/message.h>
#include <sos_shared.h>
#include "../queue.h"

/** List element used in the first level page table */
typedef struct page_entry {
	void* address;
} page_table_entry;

TAILQ_HEAD(pages_head, pit);

void pager_init(void);
void pager(L4_ThreadId_t, L4_Msg_t*);
int pager_unmap_all(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf);
void pager_free_all(L4_ThreadId_t);
void* pager_physical_lookup(L4_ThreadId_t, L4_Word_t addr);


#endif /* PAGER_H_ */
