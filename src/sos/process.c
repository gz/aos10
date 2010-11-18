#include <assert.h>
#include "process.h"

inline pid_t tid2pid(L4_ThreadId_t tid) {
	return tid.global.X.thread_no;
}

process* get_process(L4_ThreadId_t tid) {
	return &current_process;
}


void create_process(L4_ThreadId_t tid) {

	// initialize standard out file descriptor for our 1st process
	file_table_entry** file_table = get_process(L4_nilthread)->filetable;

	file_table[0] = malloc(sizeof(file_table_entry)); // freed on close()
	assert(file_table[0] != NULL);

	file_table[0]->file = file_cache[0];
	file_table[0]->mode = FM_WRITE;
	file_table[0]->owner = tid;
	file_table[0]->to_read = 0;
	file_table[0]->client_buffer = NULL;

}

