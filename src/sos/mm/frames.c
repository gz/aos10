/**
 * Frame Table
 * =============
 * This is a simple stack based frame table implementation.
 * Because it's a stack add/remove operations are both in
 * O(1). To verify if a frame is currently free/used we implemented
 * a bit-field which tracks all frames. This allows us to check the state
 * of a given frame in O(1).
 * The Memory used for the frame management is sizeof(frame_t) (i.e. 4 bytes)
 * + 1 bit in the bit-field.
 */

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include "../libsos.h"
#include "../l4.h"

#include "frames.h"

#define verbose 1

/** Reduces the number of frames to 10 */
#define SWAP_TEST 1
#define BITS_PER_CHAR 8

/** List elements used to maintain a list of the free frames */
typedef struct frame {
	L4_Word_t address; /*!< Frame start address */
} frame_t;

static L4_Word_t start; /**< Start address of physical memory */
static L4_Word_t end;	/**< Last address of physical memory */
static L4_Word_t stack_count = 0; /**< Holds the current number of stack elements */

static frame_t* frame_stack_start = NULL;
static char* bitfield_start = NULL;


/**
 * Pushes a frame on the stack.
 *
 * @param frame_address address which is pushed on the stack
 */
static void frame_stack_add(L4_Word_t frame_address) {
	(frame_stack_start+(stack_count++))->address = frame_address;
}

/**
 * Removes a Frame address from the stack.
 *
 * @return Address of the removed frame.
 */
static L4_Word_t frame_stack_remove(void) {
	assert(stack_count >= 1);
	return (frame_stack_start+(--stack_count))->address;
}

/**
 * Checks if the given parameter `frame` is within the memory range and
 * if the address is always at the start of a given frame.
 *
 * @param frame address of the frame
 * @return A boolean depending on if the address is valid or not.
 */
static L4_Bool_t is_valid_frame_address(L4_Word_t frame) {
	L4_Bool_t address_in_range = start <= frame && frame < end;
	L4_Bool_t address_is_aligned = (frame % PAGESIZE) == 0;

	return address_in_range && address_is_aligned;
}


/**
 * Sets the used bit of a given frame to the value in `bit`.
 *
 * @param frame which frame to set
 * @param bit low or high
 */
static void bitfield_set(L4_Word_t frame, L4_Bool_t bit) {
	assert(is_valid_frame_address(frame));

	L4_Word_t bit_number = ((frame-start)/PAGESIZE);
	L4_Word_t char_offset = bit_number / BITS_PER_CHAR;
	L4_Word_t bit_offset = bit_number % BITS_PER_CHAR;

	char* bitmask = bitfield_start + char_offset;
	if(bit) {
		*bitmask |= 1 << bit_offset;
	}
	else {
		*bitmask &= ~(1 << bit_offset);
	}

}

/**
 * Gets the value for the given `frame` in the bitfield.
 *
 * @return 0 or 1 depending on wheter a `frame` is marked as used or
 * not in the frame table.
 */
static int bitfield_get(L4_Word_t frame) {
	assert(is_valid_frame_address(frame));

	L4_Word_t bit_number = ((frame-start)/PAGESIZE);
	L4_Word_t char_offset = bit_number / BITS_PER_CHAR;
	L4_Word_t bit_offset = bit_number % BITS_PER_CHAR;

	char* bitmask = bitfield_start + char_offset;

	return (*bitmask & (1 << bit_offset)) > 0;

}


/**
 * This function prints the current bitfield up to
 * a given number of bits. Used for debugging.
 *
 * @param first where to start printing in the bitfield
 * @param number_of_bits #bits printed beginning from first
 *//*
static void print_bitfield(L4_Word_t first, L4_Word_t number_of_bits) {
	dprintf(0, "Printing Bitfield from %d to %d:\n", first, first+number_of_bits-1);
	int i;
	for(i=0; i<number_of_bits; i++) {
		dprintf(0, "%d", bitfield_get(start+(first+i)*PAGESIZE));
		if((i+1) % 10 == 0)
			dprintf(0, "\n");
	}
}*/

/**
 * Initialize the frame table.
 *
 * @param low lowest physical memory address
 * @param high highest physical memory address
 */
void frame_init(L4_Word_t low, L4_Word_t high) {
	start = low;
	end = high;
	L4_Word_t memsize = end - start;

	// preconditions
	assert(low < high);
	assert(memsize % PAGESIZE == 0); // our code is based on that assumption

	L4_Word_t frame_count = memsize / PAGESIZE;
	L4_Word_t frame_table_size = frame_count * sizeof(frame_t);
	L4_Word_t bit_field_size = (frame_count / 8) + 1;

	dprintf(2, "Physical Memory starts at address: %d\n", start);
	dprintf(2, "Physical Memory ends at address: %d\n", end);
	dprintf(2, "Memory Size: %d bytes\n", memsize);
	dprintf(2, "Frame Count: %d frames\n", frame_count);
	dprintf(2, "Frame List Structure size: %d bytes\n", sizeof(frame_t));
	dprintf(2, "Frame Table Size: %d bytes\n", frame_table_size);
	dprintf(2, "Bit field size: %d bytes\n",  bit_field_size);

	frame_stack_start = (frame_t*) malloc(frame_table_size); // this is never freed but it's ok
	assert(frame_stack_start != NULL);
	stack_count = 0;

	//bitfield_start = (char*) (frame_stack_start + frame_table_size);
	bitfield_start = (char*) malloc(bit_field_size); // this is never freed but it's ok
	assert(bitfield_start != NULL);

	dprintf(2, "Physical Frames will start at address: %d\n", start);

	L4_Word_t frame_iter;
	// for better debugging we add the frames in reverse order on the stack
	// so calls to frame_alloc will return with the first frames first
	for(frame_iter = end-PAGESIZE; frame_iter >= start; frame_iter -= PAGESIZE) {
		frame_stack_add(frame_iter);
		bitfield_set(frame_iter, 0);
	}

	dprintf(2, "Stack count now is: %d\n", stack_count);

	#ifdef SWAP_TEST
	stack_count = 10;
	dprintf(0, "WARNING: running with artificially decreased frame number: %d\n", stack_count);
	#endif
}


/**
 * Allocates a frame (PAGESIZE bytes) in physical memory.
 * The top element is removed from the stack and the corresponding
 * bit in the bit field is set to 1.
 *
 * @return start address of the frame or NULL in case there are no more frames left
 */
L4_Word_t frame_alloc(void) {
	if(stack_count >= 1) {
		L4_Word_t frame = frame_stack_remove();
		bitfield_set(frame, 1);
		memset((void*)frame, 0, PAGESIZE); // make sure we don't leak data to other processes
		return frame;
	}
	else {
		dprintf(0, "WARNING: %s: Ran out of physical memory :-(.\n", __FUNCTION__);
		return (L4_Word_t) NULL;
	}

}

/**
 * Frees a previously allocated frame.
 * Frame is pushed on the stack and the corresponding bit in the bit field is
 * set to 0.
 *
 * @param frame start address of frame to be freed
 */
void frame_free(L4_Word_t frame) {
	assert(is_valid_frame_address(frame));

	if(bitfield_get(frame)) {
		frame_stack_add(frame);
		bitfield_set(frame, 0);
	}
	else {
		dprintf(0, "WARNING: %s: Trying to free frame which is already free.\n", __FUNCTION__);
	}
}

