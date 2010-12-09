#include <assert.h>
#include <string.h>
#include <sos.h>

pid_t process_create(const char *path) {
	assert(strlen(path) < N_NAME);

    strcpy((char*) ipc_memory_start, path);
    L4_Msg_t msg;

    // In case there are too many concurrent processes
	// we do busy waiting until our process can be
	// created.
	pid_t pid = PROCESS_TABLE_FULL;
	do {
		L4_MsgTag_t tag = system_call(SOS_PROCESS_CREATE, &msg, 0);
		assert(L4_UntypedWords(tag) == 1);
		pid = (pid_t) L4_MsgWord(&msg, 0);

		if(pid != PROCESS_TABLE_FULL)
			return pid;

		sleep(100);
	} while(TRUE);
}


int process_delete(pid_t pid) {
    L4_Msg_t msg;
	L4_MsgTag_t tag = system_call(SOS_PROCESS_DELETE, &msg, 1, pid);
	assert(L4_UntypedWords(tag) == 1);

	return L4_MsgWord(&msg, 0);
}


pid_t my_id(void) {
    L4_Msg_t msg;
	L4_MsgTag_t tag = system_call(SOS_PROCESS_ID, &msg, 0);
	assert(L4_UntypedWords(tag) == 1);

	return L4_MsgWord(&msg, 0);
}


int process_status(process_t *processes, unsigned max) {
    L4_Msg_t msg;
	L4_MsgTag_t tag = system_call(SOS_PROCESS_STATUS, &msg, 1, max);
	assert(L4_UntypedWords(tag) == 1);

	unsigned int processes_returned = (unsigned int) L4_MsgWord(&msg, 0);
	assert(processes_returned <= max);
	process_t* processes_buffer = (process_t*) ipc_memory_start;

	for(int i=0; i<processes_returned; i++) {
		processes[i] = *processes_buffer++;
	}


	return processes_returned;
}


pid_t process_wait(pid_t pid) {
    L4_Msg_t msg;
	L4_MsgTag_t tag = system_call(SOS_PROCESS_WAIT, &msg, 1, pid);
	assert(L4_UntypedWords(tag) == 1);

	return 0;
}


int process_start(int err) {
	L4_Msg_t msg;
	system_call(SOS_PROCESS_START, &msg, 1, err);
	return 0;
}


int process_get_name(char* name) {
	// Don't do printf here as it will fuck up your ipc_memory and you need it
	// at least until after you did strcpy
	ipc_memory_start[0] = 's'; // make sure the memory is mapped somewhere

	L4_Msg_t msg;
	L4_MsgTag_t tag = system_call(SOS_PROCESS_GET_NAME, &msg, 0);
	assert(L4_UntypedWords(tag) == 1);

	int ret = L4_MsgWord(&msg, 0);
	if(ret == 0)
		strcpy(name, ipc_memory_start);

	return ret;
}
