#ifndef PROCESS_H_
#define PROCESS_H_

#include <sos_shared.h>
#include <l4/types.h>
#include <l4/message.h>
#include "io.h"
#include "pager.h"


typedef struct {
	L4_ThreadId_t tid;

	file_table_entry* filetable[PROCESS_MAX_FILES];
	page_t* pagetable;

} process;

process current_process;

process* get_process(L4_ThreadId_t tid);
pid_t tid2pid(L4_ThreadId_t);

#endif /* PROCESS_H_ */
