#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sos.h>

/**
 * Tests the behaviour of the syscalls (Blackbox testing).
 *
 * Make sure that the following files exist:
 *  - not_executable_file (without x rights)
 *  - not_readable_file (without r rights)
 *  - not_writable_file (without w rights)
 **/
int main(void) {

	///
	/// TIMER SYSCALLS
	///

	// Testing Timestamp Syscall
	uint64_t t1 = time_stamp();
	uint64_t t2 = time_stamp();
	assert(t2 > t1);

	// Testing Sleep Syscall
	t1 = time_stamp();
	sleep(100); // 100 miliseconds
	t2 = time_stamp();
	assert(t2 >= (100000 + t1) && t2 < (200000 + t1)); // 100 milliseconds = 100000 microseconds


	///
	/// PROCESS SYSCALLS
	///

	// Test process create
	assert(process_create(NULL) == EXECUTABLE_NOT_FOUND);
	assert(process_create("nonexisting file") == EXECUTABLE_NOT_FOUND);
	assert(process_create("not_executable_file") == FILE_NOT_EXECUTABLE);

	// Test process delete
	assert(process_delete(1111) == -1);
	assert(process_delete(128) == -1);
	assert(process_delete(-1) == -1);

	// Test my_id
	assert(my_id() > 0);

	// Test process_status
	process_t* processes = malloc(5*sizeof(process_t));
	assert(process_status(processes, 5) <= 5);

	// Test process wait
	pid_t tio_pid = process_create("tio");
	assert(process_wait(tio_pid) == tio_pid);
	tio_pid = process_create("tio");
	assert(process_wait(-1) == tio_pid); // make sure nothing else is running...
	assert(process_wait(1111) == -1);
	assert(process_wait(128) == -1);
	assert(process_wait(-2) == -1);
	assert(process_wait(-33) == -1);

	// Test get process name
	char name[32];
	assert(process_get_name(NULL) == -1);
	assert(process_get_name(name) == -1);


	///
	/// IO SYSCALLS
	///

	char buf[100];

	// Test open call
	assert(open(NULL, O_RDWR) == -1);

	int fd;
	assert( (fd = open("not_executable_file", O_RDWR)) >= 0);
	close(fd);
	assert(open("not_readable_file", O_RDONLY) == -1);
	assert(open("not_readable_file", O_RDWR) == -1);

	assert( (fd = open("not_writable_file", O_RDONLY)) >= 0);
	close(fd);
	assert(open("not_writable_file", O_WRONLY) == -1);
	assert(open("not_writable_file", O_RDWR) == -1);

	assert( (fd = open("console", O_RDWR)) >= 0);
	close(fd);
	pid_t c_pid = process_create("block_console");
	sleep(1000 * 10); // wait until block_console is loaded and executing

	assert( open("console", O_RDWR) == -1); // only one process can open console for reading
	assert( open("console", O_RDONLY) == -1); // only one process can open console for reading
	assert( (fd = open("console", O_WRONLY)) >= 0); // writing is okay though
	close(fd);
	process_delete(c_pid);

	// now reading should work again
	assert( (fd = open("console", O_RDWR)) >= 0);
	close(fd);
	assert( (fd = open("console", O_RDONLY)) >= 0);
	close(fd);

	// Test close call
	assert(close(99) == -1);
	assert(close(PROCESS_MAX_FILES) == -1);
	assert(close(-1) == -1);
	assert(close(-33) == -1);
	fd = open("file", O_RDWR);
	assert(close(fd) == 0);

	// Test read call
	assert(read(1, buf, 10) == -1);
	assert(read(-1, buf, 10) == -1);
	assert(read(-22, buf, 10) == -1);
	assert(read(PROCESS_MAX_FILES, buf, 10) == -1);
	assert(read(99, buf, 10) == -1);

	fd = open("bootimg.bin", O_RDONLY);
	assert(fd >= 0);
	assert(read(fd, NULL, 10) == -1);
	assert(read(fd, buf, 10) == 10);
	close(fd);

	// Test write call
	assert(write(1, buf, 10) == -1);
	assert(write(-1, buf, 10) == -1);
	assert(write(-22, buf, 10) == -1);
	assert(write(PROCESS_MAX_FILES, buf, 10) == -1);
	assert(write(99, buf, 10) == -1);

	fd = open("file", O_WRONLY);
	assert(fd >= 0);
	assert(write(fd, NULL, 10) == -1);
	assert(write(fd, buf, 10) <= 10);
	close(fd);

	// Test getdirent
	assert(getdirent(0, NULL, 10) == -1); // invalid buffer
	assert(getdirent(9999, name, 10) == -1); // non existent entry
	assert(getdirent(-1, name, 10) == -1); // non existent entry
	assert(getdirent(0, name, 10) <= 10);

	// Test stat
	stat_t stat_buffer;
	assert(stat(NULL, NULL) == -1); // invalid file & buffer
	assert(stat(NULL, &stat_buffer) == -1); // invalid file
	assert(stat("nonexisting file", &stat_buffer) == -1); // invalid file
	assert(stat("console", NULL) == -1); // invalid buffer
	assert(stat("console", &stat_buffer) == 0);
}
