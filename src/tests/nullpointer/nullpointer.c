#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sos.h>

/**
 * Null pointer exception test
 **/
int main(void) {

	L4_Word_t* null_ptr = NULL;
	*(null_ptr) = 1;
}
