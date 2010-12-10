#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sos.h>

/**
 * Test programm to test syscalls. Open console in read/write
 * then blocks until it's killed.
 **/
int main(void) {

	open("console", O_RDWR);

	while(TRUE) {
		// block
	}

	// console never gets closed (is done on process_delete)
}
