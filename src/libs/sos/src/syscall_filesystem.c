#include <sos.h>
#include <l4/ipc.h>
#include "../../../sos/syscalls.h"


fildes_t stdout_fd = 0;

/*
static L4_Msg_t* syscall(int type, const char* data, int len) {

	L4_Msg_t msg;
    L4_MsgTag_t tag;

    L4_MsgClear(&msg);
    L4_MsgAppendWord(&msg, (L4_Word_t) data);

    L4_Set_MsgLabel(&msg, type << 4);

    L4_MsgLoad(&msg);

	tag = L4_Call(sSystemId);
	if(!L4_IpcFailed(tag)) {
		return &msg;
	}
	else {
		printf("Syscall: %d failed.", type);
		return NULL;
	}
}*/


/**
 * Opens a file for a given access mode.
 *
 * @param path path to the file
 * @param mode access rights
 * @return File descriptor or -1 on error.
 */
fildes_t open(const char *path, fmode_t mode) {

	if(path == "console") {
		/*
		L4_Msg_t msg;
	    L4_MsgTag_t tag;

	    L4_MsgClear(&msg);
	    L4_MsgAppendWord(&msg, stdout_fd);
	    L4_MsgAppendWord(&msg, mode);
	    L4_Set_MsgLabel(&msg, SOS_OPEN << 4);
	    L4_MsgLoad(&msg);

	    tag = L4_Call(L4_Pager());
	    */

	}
	else {
		// open normal file
	}



	return 0;
}


int close(fildes_t file) {
	return 0;
}


int read(fildes_t file, char *buf, size_t nbyte) {
	return sos_read(buf, 0, nbyte, NULL);
}


int write(fildes_t file, const char *buf, size_t nbyte) {
	return sos_write(buf, 0, nbyte, NULL);
}


int getdirent(int pos, char *name, size_t nbyte) {
	return -1;
}


int stat(const char *path, stat_t *buf) {
	return 0;
}
