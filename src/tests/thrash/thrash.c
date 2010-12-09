#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sos.h>

#define NPAGES 10
#define NUMINTPERPAGE (1<<10)
static int thrash(void) {

	// array of pointers to the page-sized data chunks
	unsigned int* chunks[NPAGES];

	// allocate a LOT of memory
	for (unsigned int i = 0; i < NPAGES; i++) {
		chunks[i] = malloc(NUMINTPERPAGE*sizeof(unsigned int));
	    // check that we are not in physical memory
	    assert((void *) chunks[i] > (void *) 0x2000000);
		// write numbers into it
	    unsigned int chunknr = NUMINTPERPAGE * i;
		for (unsigned int j = 0; j < NUMINTPERPAGE; j++) {
			chunks[i][j] = chunknr + j;
		}
	}

	// verify the written memory and free it again
	for (unsigned int i = 0; i < NPAGES; i++) {
		// check numbers in the memory
	    unsigned int chunknr = NUMINTPERPAGE * i;
		for (unsigned int j = 0; j < NUMINTPERPAGE; j++) {
			assert(chunks[i][j] == chunknr + j);
		}
		// free current chunk
		free(chunks[i]);
	}

    return 0;
}


/**
 * This process never finishes. Used to test
 * the kill command.
 **/
int main(void) {
	thrash();
}
