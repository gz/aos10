#include <assert.h>
#include <sos.h>

uint64_t time_stamp(void) {
    L4_Msg_t msg;
	L4_MsgTag_t tag = system_call(SOS_TIMESTAMP, &msg, 0);
	assert(L4_UntypedWords(tag) == 2);

	uint64_t timestamp = ( ((uint64_t)L4_MsgWord(&msg, 1)) << 32) | L4_MsgWord(&msg, 0); // construct 64bit value from two 32bit words
	return timestamp;
}


void sleep(int msec) {
    L4_Msg_t msg;
	L4_MsgTag_t tag = system_call(SOS_SLEEP, &msg, 1, msec);
	assert(L4_UntypedWords(tag) == 1);
}
