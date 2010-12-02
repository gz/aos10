/**
 * TODO: process delete remove pending sleep timers
 * TODO: manage thread ids
 */

#include <assert.h>
#include <string.h>
#include "process.h"
#include "libsos.h"

#define verbose 2

static L4_Word_t next_task;
#define RESERVED_ID_BITS 10
#define MAX_RUNNING_PROCESS 128
static process ptable[MAX_RUNNING_PROCESS];

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


static inline pid_t tid2pid(L4_ThreadId_t tid) {
	return L4_ThreadNo(tid) >> RESERVED_ID_BITS;
}


static inline L4_ThreadId_t pid2tid(pid_t pid) {
	assert(pid >=0 && pid < MAX_RUNNING_PROCESS);
	assert(ptable[pid].is_active);

	return ptable[pid].tid;
}


static void wait_wakeup(L4_ThreadId_t finished_thread) {

	// find process with matching tid in list
	for (int i=0; i<MAX_RUNNING_PROCESS; i++) {
		process* p = &ptable[i];
		if(p != NULL && !L4_IsNilThread(p->wait_for) && (L4_IsThreadEqual(p->wait_for, finished_thread) || L4_IsThreadEqual(p->wait_for, L4_anythread))) {
			dprintf(0, "waking up p:0x%X because it waits for:0x%X", p->tid, p->wait_for);
			send_ipc_reply(p->tid, SOS_PROCESS_WAIT, 1, tid2pid(finished_thread));
			p->wait_for = L4_nilthread;
		}
	}

}


static process* allocate_process_entry(void) {

	if(next_task < MAX_RUNNING_PROCESS && ptable[next_task].is_active == FALSE) {
		return &ptable[next_task++];
	}

	// in case next given entry is not free fall back to full table search
	// we start at 1 because the first entry is used by sos
	for(int i=1; i<MAX_RUNNING_PROCESS; i++) {

		if(ptable[i].is_active == FALSE) {
			return &ptable[i];
		}

	}

	return NULL;
}

void process_init() {
	next_task = 0; // because root is 0

	// initialize process table
	for(int i=0; i<MAX_RUNNING_PROCESS; i++) {
		ptable[i].is_active = FALSE;
		ptable[i].page_index = NULL;
		ptable[i].size = 0;
		ptable[i].start_time = 0ULL;
		ptable[i].wait_for = L4_nilthread;

		volatile int threadNo = i << RESERVED_ID_BITS;
		ptable[i].tid = L4_GlobalId(threadNo, 0);
	}

	// add root process as first process in table
	register_process("[sos]");
	ptable[0].tid = root_thread_g;
}


process* get_process(L4_ThreadId_t tid) {
	pid_t pid = tid2pid(tid);
	assert(pid >= 0 && pid < MAX_RUNNING_PROCESS);

	return &ptable[pid];
}


int get_pid(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {
	return set_ipc_reply(msg_p, 1, tid2pid(tid));
}


process* register_process(char* name) {
	process* new_process = allocate_process_entry();
	assert(new_process != NULL);

	// initialize process data
	strcpy(new_process->command, name);
	new_process->wait_for = L4_nilthread;
	new_process->size = 1; // TODO in elf loading replace this with binary page size
	new_process->start_time = time_stamp();
	new_process->tid.global.X.version = 1; // TODO why is this not working with increasing version number?
	new_process->is_active = TRUE;

	// set up file table with default NULL values
	for(int i=0; i<PROCESS_MAX_FILES; i++) {
		new_process->filetable[i] = NULL;
	}

	// initialize standard out file descriptor for our 1st process
	file_table_entry** file_table = new_process->filetable;
	file_table[0] = malloc(sizeof(file_table_entry));
	assert(file_table[0] != NULL);
	file_table[0]->file = file_cache[0];
	file_table[0]->mode = FM_WRITE;
	file_table[0]->owner = new_process->tid;
	file_table[0]->to_read = 0;
	file_table[0]->client_buffer = NULL;

	// initialize page index (first level page table)
	new_process->page_index = malloc(sizeof(page_table_entry)*FIRST_LEVEL_ENTRIES);
	assert(new_process->page_index != NULL);
	for(int i=0; i<FIRST_LEVEL_ENTRIES; i++)
		new_process->page_index[i].address_ptr = NULL;

	return new_process;
}


int create_process(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {
	if(buf == NULL && !L4_IsNilThread(tid))
		return IPC_SET_ERROR(-1);

	// copy executable name, make sure it's a valid string
	char name[N_NAME+1];
	memcpy(name, buf, N_NAME);
	name[N_NAME] = '\0';

	L4_BootRec_t* boot_record = find_boot_executable(name);

	if(boot_record != NULL) {
		// Start a new task with this program
		process* pentry = register_process(name);
		L4_ThreadId_t newtid = sos_task_new(
				pentry->tid,
				root_thread_g,
				(void *) L4_SimpleExec_TextVstart(boot_record),
				(void *) 0xC0000000,
				L4_Version(pentry->tid) == 1
		);

		dprintf(0, "Created task: ox%X (pentry->tid:ox%X)\n", newtid, pentry->tid);

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
	dprintf(0, "got pid to kill:%d\n", pid);
	process* to_delete = get_process(pid2tid(pid));
	if(to_delete == NULL)
		return IPC_SET_ERROR(-1);

	dprintf(0, "deleting process: 0x%X\n", to_delete->tid);

	pager_unmap_all(to_delete->tid, NULL, NULL); // remove mappings for this thread in L4
	pager_free_all(to_delete->tid); // free frames and pager memory
	free(ptable[pid].page_index);
	ptable[pid].page_index = NULL;

	// close all files & free handlers
	for(int i=0; i<PROCESS_MAX_FILES; i++) {
		if(to_delete->filetable[i] != NULL) {
			if(L4_IsThreadEqual(to_delete->filetable[i]->file->reader, to_delete->tid))
				to_delete->filetable[i]->file->reader = L4_nilthread;

			free(to_delete->filetable[i]);
			to_delete->filetable[i] = NULL;
		}
	}

	// stop (delete?) thread
	remove_timers(to_delete->tid);
	L4_AbortIpc_and_stop(to_delete->tid);
	ptable[pid].is_active = FALSE;

	wait_wakeup(to_delete->tid);

	// return to the thread only if he himself has not requested the delete
	if(!L4_IsThreadEqual(tid, to_delete->tid)) {
		set_ipc_reply(msg_p, 1, tid2pid(to_delete->tid));
		return 1;
	}
	else {
		return 0;
	}

	return 0;
}


int wait_process(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {
	process* p = get_process(tid);

	if(L4_UntypedWords(msg_p->tag) != 1 || p == NULL)
		return IPC_SET_ERROR(-1);

	pid_t pid = L4_MsgWord(msg_p, 0);

	if(pid == -1) {
		p->wait_for = L4_anythread;
	}
	else {
		process* wait_process = get_process(pid2tid(pid));
		if(wait_process == NULL) {
			return set_ipc_reply(msg_p, SOS_PROCESS_WAIT, 1, pid);
		}

		p->wait_for = wait_process->tid;
	}

	return 0;
}


int get_process_status(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {

	if(L4_UntypedWords(msg_p->tag) != 1 || buf == NULL)
		return IPC_SET_ERROR(0);

	L4_Word_t max_processes = L4_MsgWord(msg_p, 0);
	max_processes = min(max_processes, PAGESIZE / sizeof(process_t));

	unsigned int added = 0;
	process_t* process_desc_ptr = (process_t*) buf;
	for (int i=0; i<MAX_RUNNING_PROCESS; i++) {
		if(ptable[i].is_active) {
			process* p = &ptable[i];

			strcpy(process_desc_ptr->command, p->command);
			process_desc_ptr->pid = tid2pid(p->tid);
			process_desc_ptr->size = p->size;
			process_desc_ptr->stime = (unsigned) ((time_stamp() - p->start_time) / 1000);
			process_desc_ptr->ctime = 0;

			if(++added == max_processes)
				break;

			process_desc_ptr++;
		}
	}

	return set_ipc_reply(msg_p, 1, added);
}
