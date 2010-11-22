#include <assert.h>
#include "process.h"
#include "libsos.h"

#define verbose 1

static LIST_HEAD(listhead, proc) process_head;

void process_init() {
	LIST_INIT(&process_head);
}

inline pid_t tid2pid(L4_ThreadId_t tid) {
	return L4_ThreadNo(tid);
}

process* get_process(L4_ThreadId_t tid) {

	// find process with matching tid in list
	for (process* p = process_head.lh_first; p != NULL; p = p->entries.le_next) {
		if(L4_IsThreadEqual(tid, p->tid))
			return p;
	}

	return NULL; // No process for given thread ID exists
}


void create_process(L4_ThreadId_t tid) {

	process* new_process = malloc(sizeof(process)); // TODO free on process delete
	assert(new_process != NULL);

	new_process->tid = tid;

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

	LIST_INSERT_HEAD(&process_head, new_process, entries);
}

