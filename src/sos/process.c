/**
 * Process
 * =============
 * This holds syscall handlers for process creation & deletion as well
 * as syscalls to get information about a process.
 *
 * At any point in time the process table (ptable) holds all currently
 * running processes in the system. A process is inserted in the ptable
 * through process_create and stopped (and deleted from the ptable)
 * through process delete. The ptable is a hash table using
 * the pid of a process as a key. The key is directly computed from the
 * L4 thread id of a process.
 * The size of the ptable limits the number of processes which can
 * run concurrently on the system (see MAX_RUNNING_PROCESS).
 * The pid is unique overall concurrently running processes.
 */

#include <assert.h>
#include <string.h>
#include <l4/cache.h>
#include "process.h"
#include "libsos.h"

#define verbose 1

static L4_Word_t next_task;
#define RESERVED_ID_BITS 10
#define MAX_RUNNING_PROCESS 128
/** Table containing currently running processes */
static process ptable[MAX_RUNNING_PROCESS];


/**
 * Finds a executable placed within in the bootimage file.
 * @param name executable name
 * @return boot record pointer for the executable or NULL if not found
 */
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


/**
 * Converts a given thread id to the corresponding pid.
 * @param tid Thread ID
 * @return pid for the thread
 */
static inline pid_t tid2pid(L4_ThreadId_t tid) {
	return L4_ThreadNo(tid) >> RESERVED_ID_BITS;
}


/**
 * Returns the Thread ID of a given pid (does a lookup in the
 * ptable since we precomputed the ids on process_init.
 * @param pid
 * @return Thread ID
 */
static inline L4_ThreadId_t pid2tid(pid_t pid) {
	assert(pid >=0 && pid < MAX_RUNNING_PROCESS);
	//assert(ptable[pid].is_active);

	return ptable[pid].tid;
}


/**
 * Wakes up all threads waiting either for the finished_thread
 * or just any thread.
 * @param finished_thread
 */
static void wait_wakeup(L4_ThreadId_t finished_thread) {

	// find process with matching tid in list
	for (int i=0; i<MAX_RUNNING_PROCESS; i++) {
		process* p = &ptable[i];

		if(p->is_active && !L4_IsNilThread(p->wait_for) && (L4_IsThreadEqual(p->wait_for, finished_thread) || L4_IsThreadEqual(p->wait_for, L4_anythread))) {
			dprintf(0, "waking up p:0x%X because it waits for:0x%X", p->tid, p->wait_for);
			send_ipc_reply(p->tid, SOS_PROCESS_WAIT, 1, tid2pid(finished_thread));
			p->wait_for = L4_nilthread;
		}
	}

}


/**
 * Finds a free spot in the process table and returns a pointer
 * to it. Note that a free spot in the process table is when
 * is_active is false.
 * The search algorithm uses a counter variable which is incremented
 * every time this function is called if this doesn't work
 * a fallback which does a linear search through the table is used.
 *
 * @return pointer to free entry or NULL if process table is full
 */
static process* allocate_process_entry(void) {

	if(next_task < MAX_RUNNING_PROCESS && ptable[next_task].is_active == FALSE) {
		return &ptable[next_task++];
	}
	if(next_task == MAX_RUNNING_PROCESS)
		next_task = 2; // reset

	// in case next given entry is not free fall back to full table search
	// we start at 1 because the first entry is used by sos
	for(int i=1; i<MAX_RUNNING_PROCESS; i++) {

		if(ptable[i].is_active == FALSE) {
			return &ptable[i];
		}

	}

	return NULL;
}


/**
 * Initializes the process table with default values.
 */
void process_init() {
	next_task = 0; // because root is 0

	// initialize process table
	for(int i=0; i<MAX_RUNNING_PROCESS; i++) {
		ptable[i].is_active = FALSE;
		ptable[i].initialized = FALSE;
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
	free(ptable[0].page_index);
	ptable[0].page_index = NULL;
}


/**
 * Finds a process in the ptable.
 * @param tid Thread ID of the process we want
 * @return pointer to the ptable entry
 */
process* get_process(L4_ThreadId_t tid) {
	pid_t pid = tid2pid(tid);
	assert(pid >= 0 && pid < MAX_RUNNING_PROCESS);

	return &ptable[pid];
}


/**
 * Syscall handler for my_id()
 *
 * @param tid Thread ID Callee
 * @param msg_p IPC message
 * @param buf shared memory (not used)
 * @return 1
 */
int get_pid(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {
	return set_ipc_reply(msg_p, 1, tid2pid(tid));
}


/**
 * Registers a executable within the process table.
 * @param name of the executable
 * @return pointer to the process entry or NULL if process table
 * is full
 */
process* register_process(char* name) {
	process* new_process = allocate_process_entry();
	if(new_process == NULL)
		return NULL;

	// initialize process data
	strcpy(new_process->command, name);
	new_process->wait_for = L4_nilthread;
	new_process->size = 1; // TODO in elf loading replace this with binary page size
	new_process->start_time = get_time_stamp();
	new_process->tid.global.X.version = 1; // TODO why is this not working with increasing version number?
	new_process->is_active = TRUE;
	new_process->initialized = FALSE;

	// set up file table with default NULL values
	for(int i=0; i<PROCESS_MAX_FILES; i++) {
		new_process->filetable[i] = NULL;
	}

	// initialize standard out file descriptor
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


/**
 * Syscall handler for creating a process. This will perform the following steps:
 * 1. Find the executable in the boot image
 * 2. Setup a entry in the process table
 * 3. Tell L4 to start the process
 *
 * @param tid Callee Thread ID
 * @param msg_p IPC message
 * @param buf Shared IPC memory (containing name of the executable)
 * @return 1 (send a reply to the callee)
 */
int create_process(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {
	if(buf == NULL)
		return IPC_SET_ERROR(EXECUTABLE_NOT_FOUND);

	// copy executable name, make sure it's a valid string
	char name[N_NAME+1];
	memcpy(name, buf, N_NAME);
	name[N_NAME] = '\0';
	dprintf(0, "name is:%s\n", name);
	int file_cache_index = find_file(name);

	if(file_cache_index == -1) {
		dprintf(0, "The file you're trying to execute has no executable rights.\n");

		return IPC_SET_ERROR(EXECUTABLE_NOT_FOUND);
	}
	else if(! (file_cache[file_cache_index]->status.st_fmode & FM_EXEC) ) {
		dprintf(0, "The file you're trying to execute has no executable rights.\n");
		return IPC_SET_ERROR(FILE_NOT_EXECUTABLE);
	}

	L4_BootRec_t* boot_record = find_boot_executable("initializer");

	if(boot_record != NULL && file_cache_index != -1) {
		// Start a new task with this program
		process* pentry = register_process(name);
		if(pentry == NULL)
			return IPC_SET_ERROR(PROCESS_TABLE_FULL);

		L4_ThreadId_t newtid = sos_task_new(
				pentry->tid,
				root_thread_g,
				(void *) L4_SimpleExec_TextVstart(boot_record),
				(void *) STACK_TOP,
				L4_Version(pentry->tid) == 1
		);

		dprintf(0, "Created task: ox%X (pentry->tid:ox%X) pid:%d\n", newtid, pentry->tid, tid2pid(newtid));
		return set_ipc_reply(msg_p, 1, tid2pid(newtid));
	}

	assert("Initializer not found in bootimg.bin. Check SConstruct!");
	return IPC_SET_ERROR(-1);
}


int start_process(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {
	process* p = get_process(tid);

	if(p == NULL || p->initialized)
		return IPC_SET_ERROR(-1);

	p->initialized = TRUE;

	//L4_AbortIpc_and_stop_Thread(tid);
	//L4_CacheFlushAll();
	L4_Start_SpIp(tid, STACK_TOP, ELF_START);

	return 0;
}


/**
 * Syscall handler for process_delete. We make sure to free all memory
 * a process has allocated in the server during it's runtime is freed
 * and no future callback function will call this thread anymore (in
 * case kill is called while a process was waiting in a syscall).
 * Syscalls where this can happen are sleep, nfs_write, nfs_read
 * and nfs_create. All these cases are handled accordingly in this
 * function. Note that process wait does not require special handling
 * since we reset the wait_for attribute in the register thread.
 *
 * @param tid Thread ID of callee
 * @param msg_p IPC message
 * @param buf Shared IPC memory (not used)
 * @return If the process kills itself we don't send a reply (since
 * we aborted before) otherwise we send back the killed pid.
 */
int delete_process(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {

	if(L4_UntypedWords(msg_p->tag) != 1)
		return IPC_SET_ERROR(-1);

	pid_t pid = L4_MsgWord(msg_p, 0);
	if(pid == 0)
		return IPC_SET_ERROR(-1);

	process* to_delete = get_process(pid2tid(pid));
	if(!to_delete->is_active)
		return IPC_SET_ERROR(-1);

	dprintf(2, "Deleting process: 0x%X\n", to_delete->tid);

	pager_unmap_all(to_delete->tid, NULL, NULL);
	pager_free_all(to_delete->tid); // free frames and pager memory
	free(ptable[pid].page_index);	// free 1st level page index
	ptable[pid].page_index = NULL;

	// close all files & free handlers
	for(int i=0; i<PROCESS_MAX_FILES; i++) {
		if(to_delete->filetable[i] != NULL) {
			if(to_delete->filetable[i]->awaits_callback)
				// process currently is in a read/write syscall
				// since we pass around this pointer for the
				// write back calls we cannot free it yet
				// (it's done after the callback complete)
				to_delete->filetable[i]->awaits_callback = FALSE;
			else {
				if(L4_IsThreadEqual(to_delete->filetable[i]->file->reader, to_delete->tid))
					to_delete->filetable[i]->file->reader = L4_nilthread;

				free(to_delete->filetable[i]);
				to_delete->filetable[i] = NULL;
			}

		}
	}

	// check if there are any file creation for this process pending
	// if yes, see to it that we don't reply back
	for(int i=0; i<DIR_CACHE_SIZE; i++) {
		if(file_cache[i] != NULL && file_cache[i]->creation_pending && L4_IsThreadEqual(to_delete->tid, file_cache[i]->reader))
			file_cache[i]->reader = L4_nilthread;
	}

	// remove pending sleep timers for this process
	remove_timers(to_delete->tid);

	// stop (delete?) thread
	L4_AbortIpc_and_stop(to_delete->tid);
	ptable[pid].is_active = FALSE;

	// wakup all processes waiting on this thread
	wait_wakeup(to_delete->tid);

	// return to the thread only if he himself has not requested the delete
	if(!L4_IsThreadEqual(tid, to_delete->tid)) {
		return set_ipc_reply(msg_p, 1, tid2pid(to_delete->tid));
	}

	return 0; // thread is killed, don't reply
}


/**
 * Syscall handler for process wait. Sets the wait_for attribute in the
 * process descriptor. We reply to this syscall in the wait_wakeup
 * function (called by a process_delete call)
 *
 * @param tid Calle Thread ID
 * @param msg_p IPC Message
 * @param buf Shared memory buffer (not used)
 * @return 0 don't send a reply (yet)
 */
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


/**
 * Process Status Syscall handler. Copies data about a
 * requested number of processes in the shared IPC memory.
 *
 * @param tid Callee Thread ID
 * @param msg_p IPC Message
 * @param buf Shared IPC memory pointer
 * @return 1 (send a reply)
 */
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
			process_desc_ptr->stime = (unsigned) ((get_time_stamp() - p->start_time) / 1000);
			process_desc_ptr->ctime = 0;

			if(++added == max_processes)
				break;

			process_desc_ptr++;
		}
	}

	return set_ipc_reply(msg_p, 1, added);
}


/**
 * Syscall handler returns the name of the executable of a given process
 * this call only works during initialization phase and should not be used
 * by clients.
 *
 * @param tid Calle Thread ID
 * @param msg_p IPC Message
 * @param buf Shared IPC memory
 * @return send empty message back
 */
int get_executable_name(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf) {
	process* p = get_process(tid);

	if(p == NULL || buf == NULL || p->initialized)
		return IPC_SET_ERROR(-1);

	strcpy(buf, p->command);
	return set_ipc_reply(msg_p, 1, 0);
}

