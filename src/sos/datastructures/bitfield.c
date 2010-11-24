#include "bitfield.h"
#include "../libsos.h"

#define verbose 1

/**
 * Sets a bit in the bit field to a given value.
 *
 * @param bitfield pointer to the bit field
 * @param bit_number which bit to set
 * @param value 1 or 0
 */
void bitfield_set(data_ptr bitfield, L4_Word_t bit_number, L4_Bool_t value) {

	L4_Word_t char_offset = bit_number / BITS_PER_CHAR;
	L4_Word_t bit_offset = bit_number % BITS_PER_CHAR;

	char* bitmask = bitfield + char_offset;

	// TODO some nice bit hacks could probably remove this if
	if(value) {
		*bitmask |= 1 << bit_offset;
	}
	else {
		*bitmask &= ~(1 << bit_offset);
	}

}


/**
 * Gets the high/low of a specific bit in a bit field.
 *
 * @return value of the bit
 */
L4_Bool_t bitfield_get(data_ptr bitfield, L4_Word_t bit_number) {

	L4_Word_t char_offset = bit_number / BITS_PER_CHAR;
	L4_Word_t bit_offset = bit_number % BITS_PER_CHAR;

	char* bitmask = bitfield + char_offset;

	return (*bitmask & (1 << bit_offset)) > 0;
}


/**
 * This function prints the current bitfield up to
 * a given number of bits. Used for debugging.
 *
 * @param first where to start printing in the bitfield
 * @param number_of_bits #bits printed beginning from first
 */
void print_bitfield(data_ptr bitfield, L4_Word_t first, L4_Word_t number_of_bits) {
	dprintf(0, "Printing Bitfield from %d to %d:\n", first, first+number_of_bits-1);
	int i;
	for(i=0; i<number_of_bits; i++) {
		dprintf(0, "%d", bitfield_get(bitfield, first+i));
		if((i+1) % 10 == 0)
			dprintf(0, "\n");
	}
}

