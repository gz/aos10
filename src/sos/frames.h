#ifndef _FRAMES_H
#define _FRAMES_H

#include <l4/types.h>

void frame_init(L4_Word_t low, L4_Word_t high);
L4_Word_t frame_alloc(void);
void frame_free(L4_Word_t frame);

void print_bitfield(L4_Word_t start, L4_Word_t end);


#endif // _FRAMES_H
