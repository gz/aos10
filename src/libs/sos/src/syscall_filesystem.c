#include <sos.h>

fildes_t stdout_fd = 0;


fildes_t open(const char *path, fmode_t mode) {
	return -1;
}


int close(fildes_t file) {
	return 0;
}


int read(fildes_t file, char *buf, size_t nbyte) {
	return 0;
}


int write(fildes_t file, const char *buf, size_t nbyte) {
	//sos_write(buf)
	return 0;
}


int getdirent(int pos, char *name, size_t nbyte) {
	return -1;
}


int stat(const char *path, stat_t *buf) {
	return 0;
}
