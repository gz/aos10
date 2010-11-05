#include <assert.h>
#include <sos.h>

long time_stamp(void) {
    L4_Msg_t msg;
	L4_MsgTag_t tag = system_call(SOS_TIMESTAMP, &msg, 0);
	assert(L4_UntypedWords(tag) == 1);
	return 0;
}


void sleep(int msec) {
    L4_Msg_t msg;
	L4_MsgTag_t tag = system_call(SOS_SLEEP, &msg, 1, msec);
	assert(L4_UntypedWords(tag) == 1);
}
