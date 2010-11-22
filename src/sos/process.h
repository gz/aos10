#ifndef PROCESS_H_
#define PROCESS_H_

#include <sos_shared.h>
#include <l4/types.h>
#include <l4/message.h>

#include "queue.h"
#include "io/io.h"
#include "mm/pager.h"

typedef struct proc {
	LIST_ENTRY(proc) entries;

	L4_ThreadId_t tid;
	file_table_entry* filetable[PROCESS_MAX_FILES];
	page_table_entry* pagetable;

} process;

process* current_process;

void process_init(void);
process* get_process(L4_ThreadId_t tid);
void create_process(L4_ThreadId_t tid);
pid_t tid2pid(L4_ThreadId_t);

#endif /* PROCESS_H_ */
