#ifndef PROCESS_H_
#define PROCESS_H_

#include <sos_shared.h>
#include <l4/types.h>
#include <l4/message.h>
#include <clock.h>

#include "io/io.h"
#include "mm/pager.h"

/** Process Descriptor entries */
typedef struct proc {
	L4_Bool_t initialized;		/**< Set when ELF binary has been loaded and process code is being executed */
	L4_Bool_t is_active;		/**< Determine if the process is active */

	char command[N_NAME];		/**< Name of the executable */

	L4_ThreadId_t tid;			/**< ID of the thread running the process */
	L4_ThreadId_t wait_for;		/**< Used by process_wait calls */

	L4_Word_t  size;			/**< Used pages */
	timestamp_t  start_time;	/**< Start time of the process */

	file_table_entry* filetable[PROCESS_MAX_FILES];		/**< Filetable */
	page_table_entry* page_index;						/**< 1st level page table */
} process;

#define MAX_RUNNING_PROCESS 128

int create_process(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int start_process(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int delete_process(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int wait_process(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int get_pid(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int get_process_status(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int get_executable_name(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf);

void process_init(void);
process* get_process(L4_ThreadId_t tid);
process* register_process(char* name);
inline L4_ThreadId_t pid2tid(pid_t);
inline pid_t tid2pid(L4_ThreadId_t);

#endif /* PROCESS_H_ */
