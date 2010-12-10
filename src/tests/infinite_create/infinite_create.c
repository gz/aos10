#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sos.h>

/**
 * Executes create file a lot. Used to test if the kill
 * command works correctly during open() (specifically creation)
 * calls.
 **/
int main(void) {

	for(int i=65; i<90; i++) {
		for(int j=65; j<90; j++) {
			for(int k=65; k<90; k++) {

				// generate file name
				char name[4];
				name[0] = (char) i;
				name[1] = (char) j;
				name[2] = (char) k;
				name[4] = '\0';

				int fd = open(name, O_RDONLY);
				assert(fd >= 0);
				close(fd);

			}
		}
	}

}
