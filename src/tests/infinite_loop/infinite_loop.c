#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sos.h>

/**
 * This process never finishes. Used to test
 * the kill command.
 **/
int main(void) {

	printf("Infinite Looping Process is running... utcb at: %p\n", L4_GetUtcbBase());

	while(1) {
		sleep(1000);
	}

}
