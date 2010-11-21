#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "frames.h"
#include "frames_test.h"

/* Allocate 10 frames and make sure you can touch them all */
void frame_test1(void) {
	int i;
	L4_Word_t* frame;

	for (i = 0; i < 10; i++) {
		/* Allocate a frame */
		frame = (L4_Word_t *) frame_alloc();
		assert(frame);

		/* Test you can touch the frame */
		*frame = 0x37;
		assert(*frame == 0x37);

		printf("Frame #%d allocated at %p\n",  i, frame);
	}
}

/* Test that you eventually run out of memory gracefully,
   and doesn't crash */
void frame_test2(void) {
	L4_Word_t* frame;
	//int i = 0;
	for (;;) {
		 /* Allocate a frame */
		 frame = (L4_Word_t *) frame_alloc();
		 //printf("Frame #%d allocated at %p\n",  i++, frame);

		 if (!frame) {
		  printf("Out of memory!\n");
		  break;
		 }

		 /* Test you can touch the frame */
		 *frame = 0x37;
		 assert(*frame == 0x37);
	}
}


/* Test that you never run out of memory if you always free frames.
    This loop should never finish */
void frame_test3(void) {
	L4_Word_t* frame;
	int i = 0;
	for (;;) {
		 /* Allocate a frame */
		 frame = (L4_Word_t *) frame_alloc();
		 assert(frame != NULL);

		 /* Test you can touch the frame */
		 *frame = 0x37;
		 assert(*frame == 0x37);

		 printf("Frame #%d allocated at %p\n",  i++, frame);

		 frame_free((L4_Word_t) frame);
	}
}

/* Double freeing a frame */
void frame_test4(void) {
	int i;
	L4_Word_t* frame;

	for (i = 0; i < 10; i++) {
		/* Allocate a frame */
		frame = (L4_Word_t *) frame_alloc();
		assert(frame);

		/* Test you can touch the frame */
		*frame = 0x37;
		assert(*frame == 0x37);
		printf("Frame #%d allocated at %p\n",  i, frame);
		print_bitfield(0, 10);
	}

	for(i=0; i< 10; i++) {
		frame_free((L4_Word_t)frame);
		frame = (L4_Word_t*) (((L4_Word_t) frame) - 4096);
		print_bitfield(0, 10);
	}
}
