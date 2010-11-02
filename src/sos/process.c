#include "process.h"

inline pid_t tid2pid(L4_ThreadId_t tid) {
	return tid.global.X.thread_no;
}

process* get_process(L4_ThreadId_t tid) {
	return &current_process;
}

/*
void create_process(L4_ThreadId_t tid) {
	//current_process.filetable[0].identifier = "console";
}
*/
