#include <assert.h>
#include <string.h>
#include "process.h"
#include "libsos.h"

#define verbose 2

static unsigned int task;
static LIST_HEAD(listhead, proc) process_head;

static L4_BootRec_t* find_boot_executable(const char* name) {

	// Loop through the BootInfo starting executables
    int i;
    L4_BootRec_t* binfo_rec;
    for (i = 1; (binfo_rec = sos_get_binfo_rec(i)); i++) {

    	if (L4_BootRec_Type(binfo_rec) != L4_BootInfo_SimpleExec)
			continue;

    	if(strcmp(L4_SimpleExec_Cmdline(binfo_rec), name) == 0) {
			dprintf(1, "Found executable for process create: %d %s\n", i, L4_SimpleExec_Cmdline(binfo_rec));
    		return binfo_rec;
    	}

    }

    return NULL;
}

void process_init() {
	LIST_INIT(&process_head);
	task = 0;
}

inline pid_t tid2pid(L4_ThreadId_t tid) {
	return L4_ThreadNo(tid);
}

static inline L4_ThreadId_t pid2tid(pid_t pid) {
	return L4_GlobalId(pid, 1);
}

process* get_process(L4_ThreadId_t tid) {

	// find process with matching tid in list
	for (process* p = process_head.lh_first; p != NULL; p = p->entries.le_next) {
		if(L4_IsThreadEqual(tid, p->tid))
			return p;
	}

	return NULL; // No process for given thread ID exists
}


int get_pid(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {
	return set_ipc_reply(msg_p, 1, L4_ThreadNo(tid));
}


int create_process(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {
	if(buf == NULL && !L4_IsNilThread(tid))
		return IPC_SET_ERROR(-1);

	// copy executable name, make sure it's a valid string
	char name[MAX_PATH_LENGTH+1];
	memcpy(name, buf, MAX_PATH_LENGTH+1);
	name[MAX_PATH_LENGTH] = '\0';

	L4_BootRec_t* boot_record = find_boot_executable(name);

	if(boot_record != NULL) {
		// Start a new task with this program
		L4_ThreadId_t newtid = sos_task_new(
				++task,
				root_thread_g,
				(void *) L4_SimpleExec_TextVstart(boot_record),
				(void *) 0xC0000000
		);
		dprintf(0, "Created task: ox%X\n", newtid);

		if(!L4_IsNilThread(tid)) {
			dprintf(0, "returning with pid:%d\n", tid2pid(newtid));
			return set_ipc_reply(msg_p, 1, tid2pid(newtid));
		}
	}

	if(!L4_IsNilThread(tid))
		return IPC_SET_ERROR(-1); // executable not found

	return 0; // return if not called from syscall context
}


int delete_process(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {

	if(L4_UntypedWords(msg_p->tag) != 1)
		return IPC_SET_ERROR(-1);

	pid_t pid = L4_MsgWord(msg_p, 0);
	process* to_delete = get_process(pid2tid(pid)); // TODO pid to tid
	if(to_delete == NULL)
		return IPC_SET_ERROR(-1);

	dprintf(0, "deleting process: 0x%X\n", to_delete->tid);

	pager_unmap_all(to_delete->tid, NULL, NULL); // remove mappings for this thread in L4
	pager_free_all(to_delete->tid); // free frames and page table memory

	// close all files
	// free file handlers

	// stop (delete?) thread
	// TODO remove from process list
	free(to_delete); // free process structure

	L4_AbortIpc_and_stop(to_delete->tid);
	return 0;
}


void register_process(L4_ThreadId_t tid) {

	process* new_process = malloc(sizeof(process)); // TODO free on process delete
	assert(new_process != NULL);

	// initialize process data
	new_process->tid = tid;
	new_process->size = 1; // 1 because we count the shared page for syscalls
	new_process->start_time = time_stamp();

	// set up file table with default NULL values
	for(int i=0; i<PROCESS_MAX_FILES; i++) {
		new_process->filetable[i] = NULL;
	}

	// initialize standard out file descriptor for our 1st process
	file_table_entry** file_table = new_process->filetable;
	file_table[0] = malloc(sizeof(file_table_entry)); // freed on close() OR TODO process delete
	assert(file_table[0] != NULL);
	file_table[0]->file = file_cache[0];
	file_table[0]->mode = FM_WRITE;
	file_table[0]->owner = tid;
	file_table[0]->to_read = 0;
	file_table[0]->client_buffer = NULL;

	// initialize page index (first level page table)
	for(int i=0; i<FIRST_LEVEL_ENTRIES; i++)
		new_process->page_index[i].address_ptr = 0;


	LIST_INSERT_HEAD(&process_head, new_process, entries);
}

