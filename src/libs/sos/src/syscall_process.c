#include <assert.h>
#include <sos.h>

pid_t process_create(const char *path) {
    L4_Msg_t msg;
	L4_MsgTag_t tag = system_call(SOS_PROCESS_CREATE, &msg, 0);
	assert(L4_UntypedWords(tag) == 1);

	return L4_MsgWord(&msg, 0);
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

	return 0;
}


pid_t process_wait(pid_t pid) {
    L4_Msg_t msg;
	L4_MsgTag_t tag = system_call(SOS_PROCESS_WAIT, &msg, 1, pid);
	assert(L4_UntypedWords(tag) == 1);

	return 0;
}
