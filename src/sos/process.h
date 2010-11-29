#ifndef PROCESS_H_
#define PROCESS_H_

#include <sos_shared.h>
#include <l4/types.h>
#include <l4/message.h>
#include <clock.h>

#include "queue.h"
#include "io/io.h"
#include "mm/pager.h"

typedef struct proc {
	LIST_ENTRY(proc) entries;

	L4_ThreadId_t tid;

	L4_Word_t  size;
	timestamp_t  start_time;

	file_table_entry* filetable[PROCESS_MAX_FILES];
	page_table_entry* pagetable;

} process;

int create_process(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int delete_process(L4_ThreadId_t, L4_Msg_t*, data_ptr);

void process_init(void);
process* get_process(L4_ThreadId_t tid);
void register_process(L4_ThreadId_t tid);
pid_t tid2pid(L4_ThreadId_t);

#endif /* PROCESS_H_ */
