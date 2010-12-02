#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sos.h>

/**
 * This process never finishes. Used to test
 * the kill command.
 **/
int main(void) {

	printf("Infinite Read Process is running...\n");
	char data[300];

	int fd = open("bootimg.bin", O_RDONLY);
	assert(fd >= 0);
	while(1) {
		read(fd, data, 299);
	}
	close(fd); // nice try
}
