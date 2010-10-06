#include <l4/types.h>

// List elements used to maintain a list of the free frames
struct frame {
	L4_Word_t address; // frame start address
};
typedef struct frame frame_t;



void frame_init(L4_Word_t low, L4_Word_t frame);
L4_Word_t frame_alloc(void);
void frame_free(L4_Word_t frame);

void print_bitfield(L4_Word_t, L4_Word_t);


void frame_test1(void);
void frame_test2(void);
void frame_test3(void);
void frame_test4(void);
