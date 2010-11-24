#ifndef BITFIELD_H_
#define BITFIELD_H_

#include <l4/types.h>
#include <sos_shared.h>

#define BITS_PER_CHAR 8

void bitfield_set(data_ptr, L4_Word_t, L4_Bool_t);
L4_Bool_t bitfield_get(data_ptr, L4_Word_t);
void print_bitfield(data_ptr, L4_Word_t, L4_Word_t);

#endif /* BITFIELD_H_ */
