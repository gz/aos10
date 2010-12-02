#ifndef PROCESS_H_
#define PROCESS_H_

#include <sos_shared.h>
#include <l4/types.h>
#include <l4/message.h>
#include <clock.h>

#include "io/io.h"
#include "mm/pager.h"

typedef struct proc {
	L4_Bool_t is_active;

	char command[N_NAME];

	L4_ThreadId_t tid;
	L4_ThreadId_t wait_for;

	L4_Word_t  size;
	timestamp_t  start_time;

	file_table_entry* filetable[PROCESS_MAX_FILES];
	page_table_entry* page_index;
} process;

int create_process(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int delete_process(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int wait_process(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int get_pid(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int get_process_status(L4_ThreadId_t, L4_Msg_t*, data_ptr);

void process_init(void);
process* get_process(L4_ThreadId_t tid);
process* register_process(char* name);

#endif /* PROCESS_H_ */
