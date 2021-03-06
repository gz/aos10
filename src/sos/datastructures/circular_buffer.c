#include <assert.h>
#include <string.h>
#include <l4/types.h>
#include "circular_buffer.h"
#include "../libsos.h"

#define verbose 0


int circular_buffer_read(circular_buffer* cb, int to_read, data_ptr buffer) {

	L4_Word_t max_read = min(to_read, cb->size-1);
	L4_Word_t readable = min((cb->size + cb->write_position - cb->read_position) % cb->size, max_read);

	if(readable > 0) {

		if (cb->read_position < cb->write_position)
			memcpy(buffer, &cb->buffer[cb->read_position], cb->write_position - cb->read_position);
		else {
			memcpy(buffer, &cb->buffer[cb->read_position], cb->size-cb->read_position);
			memcpy(&buffer[cb->size - cb->read_position], cb->buffer, cb->write_position);
		}

		// increment read pointer
		cb->overflow = FALSE;
		cb->read_position = (cb->read_position + readable) % cb->size;
	}


	return readable;
}


int circular_buffer_write(circular_buffer* cb, int to_write, char c) {

	assert(to_write == 1);

	if(cb->overflow) {
		dprintf(0, "Circular buffer overflow. We're starting to loose things here :-(.\n");
		return 0;
	}

	// add char to circular buffer
	cb->buffer[cb->write_position] = c;
	cb->write_position = (cb->write_position + 1) % cb->size;

	// end of the buffer (but one char) has been reached, add newline and send text
	if ((cb->write_position+1) % cb->size == cb->read_position) {
		cb->overflow = TRUE; // buffer is full for the 2nd time, drop whatever follows
	}

	return to_write;

}
