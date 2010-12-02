#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sos.h>

/**
 * This process never finishes. Used to test
 * the kill command.
 **/
int main(void) {

	printf("Infinite Write Process is running...\n");
	char data[100];
	for(int i=0; i<100; i++) {
		data[i] = 'c';
	}

	int fd = open("infinite_write", O_WRONLY);
	while(1) {
		write(fd, data, 99);
	}
	close(fd);
}
