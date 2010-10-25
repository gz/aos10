#include <sos.h>
#include <l4/ipc.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

fildes_t stdout_fd = 0;


/**
 * Opens a file for a given access mode.
 *
 * @param path path to the file
 * @param mode access rights
 * @return File descriptor or -1 on error.
 */
fildes_t open(const char *path, fmode_t mode) {

	assert(strlen(path) < 4096);

	strcpy((char*) ipc_memory_start, path);

    //L4_Msg_t msg;
	//L4_MsgTag_t tag = system_call(SOS_OPEN, &msg, 1, mode);

	//assert(L4_UntypedWords(tag) == 1);

	return 0;
}


int close(fildes_t file) {
	return 0;
}


int read(fildes_t file, char* buf, size_t to_read) {
    L4_Msg_t msg;

    (void) system_call(SOS_READ, &msg, 2, file, to_read);
	//assert(L4_UntypedWords(tag) == 1);

	L4_Word_t received = L4_MsgWord(&msg, 0);
	assert(received <= MAX_IO_BUF);

	memcpy(buf, ipc_memory_start, received);

	// debug out
	printf("received: %lu\n", received);
	char* locbuf = malloc(received+1);
	memcpy(locbuf,buf,received);
	locbuf[received] = '\0';
    printf("got string msg: %s\n", locbuf);
    free(locbuf);

	return received;
}


int write(fildes_t file, const char* buf, size_t nbyte) {
	//return sos_write(buf, 0, nbyte, NULL);

    int not_sent_count = nbyte;
  	data_ptr realdata = (data_ptr) buf;
  	data_ptr write_buffer = ipc_memory_start;

    while(not_sent_count > 0) {

        int to_send = min(MAX_IO_BUF, not_sent_count);

        // fill up buffer
        memcpy(write_buffer, realdata, to_send);
        realdata += to_send;

        L4_Msg_t msg;
    	L4_MsgTag_t tag = system_call(SOS_WRITE, &msg, 2, file, to_send);

    	// get replied data - should be one word containing the number of written chars
    	assert(L4_UntypedWords(tag) == 1);
    	int sent = L4_MsgWord(&msg, 0);

    	if(sent != to_send)
    		return -1;

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
