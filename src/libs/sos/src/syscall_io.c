#include <sos.h>
#include <l4/ipc.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

fildes_t stdout_fd = 0;


/**
 * Opens a file for a given access mode.
 * The file path is copied over IPC shared memory.
 * The mode is passed in the first IPC message register.
 *
 * @param path path to the file
 * @param mode access rights
 * @return File descriptor or -1 on error.
 */
fildes_t open(const char *path, fmode_t mode) {
	// preconditions
	assert(strlen(path) <= MAX_PATH_LENGTH);
	assert(mode == O_RDONLY || mode == O_WRONLY || mode == O_RDWR);

	//  copy path in IPC memory
	strcpy((char*) ipc_memory_start, path);

    L4_Msg_t msg;
	L4_MsgTag_t tag = system_call(SOS_OPEN, &msg, 1, mode);
	assert(L4_UntypedWords(tag) == 1);

	fildes_t fd = L4_MsgWord(&msg, 0);

	return fd;
}


int close(fildes_t file) {
	// preconditions
	assert(file >= 0);

    L4_Msg_t msg;
	L4_MsgTag_t tag = system_call(SOS_CLOSE, &msg, 1, file);
	assert(L4_UntypedWords(tag) == 1);

	int ret = L4_MsgWord(&msg, 0);

	return ret;
}


int read(fildes_t file, char* buf, size_t to_read) {

	if(buf == NULL)
		return -1;

	if(to_read == 0)
		return 0; // shortcut

    L4_Msg_t msg;
    L4_MsgTag_t tag = system_call(SOS_READ, &msg, 2, file, to_read);
	assert(L4_UntypedWords(tag) == 1);

	int received = L4_MsgWord(&msg, 0);

	if(received == -1) // error
		return -1;

	assert(received <= MAX_IO_BUF && received <= to_read);


	memcpy(buf, ipc_memory_start, received);

	return received;
}


int write(fildes_t file, const char* buf, size_t nbyte) {

	if(buf == NULL)
		return -1;

	if(nbyte == 0)
		return 0; // shortcut

    int not_sent_count = nbyte;
  	data_ptr realdata = (data_ptr) buf;
  	data_ptr write_buffer = ipc_memory_start;

    while(not_sent_count > 0) {

        int to_send = min(IPC_MEMORY_SIZE, not_sent_count);

        // fill up buffer
        memcpy(write_buffer, realdata, to_send);
        realdata += to_send;

        L4_Msg_t msg;
    	L4_MsgTag_t tag = system_call(SOS_WRITE, &msg, 2, file, to_send);
    	assert(L4_UntypedWords(tag) == 1);

    	int sent = L4_MsgWord(&msg, 0);

    	if(sent != to_send)
    		return nbyte - not_sent_count; // out of memory, don't write anymore

    	not_sent_count -= to_send;
    }

	return nbyte;
}


int getdirent(int pos, char *name, size_t nbyte) {
	return -1;
}


int stat(const char *path, stat_t *buf) {
	return 0;
}
