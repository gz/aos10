#include "swapper.h"

static clock_item* oldest = NULL;

void clock_page_insert(clock_item* ci) {

	// first item
	if(oldest == NULL) {
		oldest = ci;
		ci->next = ci;
		ci->previous = ci;
		return;
	}

	clock_item* old_prev = oldest->previous;
	ci->next = oldest;
	ci->previous = old_prev;

	oldest->previous = ci;
	old_prev->next = ci;
}

clock_item* clock_get_oldest() {
	return oldest;
}

clock_item* clock_remove_oldest() {
	clock_item* to_remove = oldest;

	oldest->previous->next = oldest->next;
	oldest->next->previous = oldest->previous;

	oldest = oldest->next;

	return to_remove;
}
