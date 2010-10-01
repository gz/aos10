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

#define verbose 2
#define max(a,b) (((a) > (b)) ? (a) : (b))

static L4_Word_t start;
static L4_Word_t end;

static frame_t* frame_stack_start = NULL;
static L4_Word_t stack_count = 0; // holds the number of stack elements


static void frame_stack_add(L4_Word_t frame_address) {
	(frame_stack_start+(++stack_count))->address = frame_address;
}

static L4_Word_t frame_stack_remove(void) {
	assert(stack_count >= 0);
	return (frame_stack_start+(stack_count--))->address;
}


/**
 * Initialise the frame table. The current implementation is
 * clearly not sufficient.
 */
void frame_init(L4_Word_t low, L4_Word_t high)
{
    start = low;
	end = high;

	L4_Word_t memsize = end - start;

	assert(memsize % PAGESIZE == 0);
	L4_Word_t frame_count = memsize / PAGESIZE;

	L4_Word_t frame_table_size = frame_count * sizeof(frame_t);
	dprintf(2, "Mem Size: %d bytes\n", memsize);
	dprintf(2, "Frame Count: %d frames\n", frame_count);
	dprintf(2, "Frame List Structure size: %d bytes\n", sizeof(frame_t));
	dprintf(2, "Frame Table Size: %d bytes\n", frame_table_size);
	//assert(frame_table_size <= PAGESIZE); Frame Table Size: 23432 bytes



	frame_stack_start = (frame_t*) start; // lets point our stack to the start
	stack_count = 0;

	start += max(PAGESIZE, frame_table_size & ~(PAGESIZE-1) ); // update so we won't overwrite the table
															   // align frame_table_size to be multiple of 4096
	L4_Word_t frame_iter;
	// initially all frames are free so we add them all on the stack
	for( frame_iter = start; frame_iter < end; frame_iter += PAGESIZE) {
		frame_stack_add(frame_iter);
	}
	assert(frame_iter == end);

}


/**
 * The top element is removed from the stack and its corresponding
 * address is returned. NULL is returned in case there are no more
 * free frames.
 */
L4_Word_t frame_alloc(void) {
	if(stack_count >= 0)
		return frame_stack_remove();
	else
		return (L4_Word_t) NULL;
}


/**
 * Frame is marked as free by pushing it to the stack.
 */
void frame_free(L4_Word_t frame) {
	frame_stack_add(frame);
}

