#include "swapper.h"

//static clock_item* oldest = NULL;

void cil_insert(clock_item* list, clock_item* ci) {
	assert(ci->next == NULL);
	assert(ci->previous == NULL);

	// first item
	if(list == NULL) {
		ci->next = ci;
		ci->previous = ci;
		return;
	}

	clock_item* old_prev = list->previous;
	ci->next = list;
	ci->previous = old_prev;

	list->previous = ci;
	old_prev->next = ci;
}

void cil_remove(clock_item* ci) {
	assert(ci->previous != NULL);
	assert(ci->next != NULL);

	clock_item* to_remove = ci;

	ci->previous->next = ci->next;
	ci->next->previous = ci->previous;

	ci = ci->next;

	return to_remove;
}
