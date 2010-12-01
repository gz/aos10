#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sos.h>

/**
 * Spawn new processes forever. Tests that everything is freed
 * correctly in the root server and we never run out of
 * thread ids.
 **/
int main(void) {
	printf("Hello World\n");
	process_create("tip");
}
