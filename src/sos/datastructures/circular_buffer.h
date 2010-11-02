/*
 * circular_buffer.h
 *
 *  Created on: Nov 1, 2010
 *      Author: gz
 */

#ifndef CIRCULAR_BUFFER_H_
#define CIRCULAR_BUFFER_H_

#include <sos_shared.h>
#include <l4/types.h>

typedef struct {
	int read_position;
	int write_position;
	data_ptr buffer;
	int size;
	L4_Bool_t overflow;

} circular_buffer;

int circular_buffer_read(circular_buffer*, int, data_ptr);
int circular_buffer_write(circular_buffer*, int, char);

#endif /* CIRCULAR_BUFFER_H_ */
