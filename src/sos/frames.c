/**
 * Frame Table
 * =============
 * This is a simple stack based frame table implementation.
 * Because it's a stack add/remove operations are both in
 * O(1). The Memory used for the frame table is currently at 4
 * byte per frame. However for simplicity we allocate a multiple of
 * 4096 bytes for the frame table
 *
 * TODO: maybe its better to allocate the frame table in the heap memory of root server?
 */

#include <stddef.h>
#include <assert.h>
#include "libsos.h"
#include "l4.h"

#include "frames.h"
#define verbose 3

#define BITS_PER_CHAR 8
#define ALIGN_TO_PAGESIZE(bytes) (((bytes)+PAGESIZE) & ~(PAGESIZE-1))

// List elements used to maintain a list of the free frames
typedef struct frame {
	L4_Word_t address; // frame start address
} frame_t;

static L4_Word_t start;
static L4_Word_t end;

static frame_t* frame_stack_start = NULL;
static char* bitfield_start = NULL;

static L4_Word_t stack_count = 0; // holds the number of stack elements

static void frame_stack_add(L4_Word_t frame_address) {
	(frame_stack_start+(stack_count++))->address = frame_address;
}

static L4_Word_t frame_stack_remove(void) {
	assert(stack_count >= 1);
	return (frame_stack_start+(--stack_count))->address;
}

/**
 * Checks if the given parameter `frame` is within the memory range and
 * if the address is always at the start of a given frame.
 */
static L4_Bool_t is_valid_frame_address(L4_Word_t frame) {
	L4_Bool_t address_in_range = start <= frame && frame < end;
	L4_Bool_t address_is_aligned = (frame % PAGESIZE) == 0;

	return address_in_range && address_is_aligned;
}


/**
 * Sets the used bit of a given frame to the value in `bit`.
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
 * Returns 0/1 depending on wheter a `frame` is marked as used or
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
 */
void print_bitfield(L4_Word_t first, L4_Word_t number_of_bits) {
	dprintf(0, "Printing Bitfield from %d to %d:\n", first, number_of_bits-1);
	int i;
	for(i=0; i<number_of_bits; i++) {
		dprintf(0, "%d", bitfield_get(start+(first+i)*PAGESIZE));
		if((i+1) % 10 == 0)
			dprintf(0, "\n");
	}
}

/**
 * Initialise the frame table. The current implementation is
 * clearly not sufficient.
 */
void frame_init(L4_Word_t low, L4_Word_t high) {
	start = low;
	end = high;
	L4_Word_t memsize = end - start;

	// preconditions
	assert(low < high);
	assert(memsize >= 2*PAGESIZE); // we need at least 2 pages
	assert(memsize % PAGESIZE == 0); // our code is based on that assumption

	L4_Word_t frame_count = memsize / PAGESIZE;
	L4_Word_t frame_table_size = frame_count * sizeof(frame_t);
	L4_Word_t bit_field_size = (frame_count / 8) + 1;
	L4_Word_t datastructure_space = ALIGN_TO_PAGESIZE(frame_table_size + bit_field_size);
	assert(datastructure_space <= memsize-PAGESIZE); // we need at least one page to manage

	dprintf(2, "Physical Memory starts at address: %d\n", start);
	dprintf(2, "Physical Memory ends at address: %d\n", end);
	dprintf(2, "Mem Size: %d bytes\n", memsize);
	dprintf(2, "Frame Count: %d frames\n", frame_count);
	dprintf(2, "Frame List Structure size: %d bytes\n", sizeof(frame_t));
	dprintf(2, "Frame Table Size: %d bytes\n", frame_table_size);
	dprintf(2, "Bit field size: %d bytes\n",  bit_field_size);
	dprintf(2, "Required memory for frame bookkeeping: %d bytes\n",  datastructure_space);
	dprintf(2, "Reserved Pages for bookkeeping: %d\n",  datastructure_space / PAGESIZE);

	frame_stack_start = (frame_t*) start; // lets point our stack to the start
	stack_count = 0;

	bitfield_start = (char*) (frame_stack_start + frame_table_size);

	start += datastructure_space; // update so we won't overwrite the table
	dprintf(2, "Physical Frames will start at address: %d\n", start);

	L4_Word_t frame_iter;
	// initially all frames are free so we add them all on the stack
	/*for( frame_iter = start; frame_iter < end; frame_iter += PAGESIZE) {
		frame_stack_add(frame_iter);
		bitfield_set(frame_iter, 0);
	}
	assert(frame_iter == end);*/

	for(frame_iter = end-PAGESIZE; frame_iter >= start; frame_iter -= PAGESIZE) {
		frame_stack_add(frame_iter);
		bitfield_set(frame_iter, 0);
	}
	assert(frame_iter == (start-PAGESIZE));

	dprintf(2, "Stack count now is: %d\n", stack_count);

}


/**
 * The top element is removed from the stack and its corresponding
 * address is returned. NULL is returned in case there are no more
 * free frames.
 */
L4_Word_t frame_alloc(void) {
	if(stack_count >= 1) {
		L4_Word_t frame = frame_stack_remove();
		bitfield_set(frame, 1);
		return frame;
	}
	else {
		dprintf(0, "WARNING: %s: Ran out of physical memory :-(.\n", __FUNCTION__);
		return (L4_Word_t) NULL;
	}

}

/**
 * Frame is marked as free by pushing it to the stack and setting the corresponding
 * bit in the bitfield to 0.
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

