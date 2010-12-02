#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sos.h>

/**
 * Make sure to spawn more than the maximum number of processes the system can handle.
 * This code here will bloat heavily.
 **/
int main(void) {
	printf("Hello World, I'm %d\n", my_id());
	sleep(5);
	for(int i=0; i<5; i++) {
		process_create("tmp");
	}
}
