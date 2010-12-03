#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sos.h>

int main(void) {
	printf("Hello World\n");
	printf("I'm test_io\n");

	/*char* buf = malloc(4096);
	assert(buf != NULL);*/
	char buf[4096];

	int fd = open("swap", O_RDONLY);
	assert(fd != -1);
	int bytes_read = read(fd, buf, 122);

	printf("Cool I read %d bytes from the swap file.\n", bytes_read);

	for(int i=0; i<1; i++) {
		printf("now sleep 1 sec\n");
		sleep(1000);
	}

	printf("test_io is done now...\n");
}
