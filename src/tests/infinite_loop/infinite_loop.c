#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sos.h>

/**
 * This process never finishes. Used to test
 * the kill command.
 **/
int main(void) {

	printf("Infinite Looping Process is running...\n");

	while(1) {
		sleep(1000);
	}

}
