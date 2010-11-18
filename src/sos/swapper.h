#ifndef SWAPPER_H_
#define SWAPPER_H_

struct clit;
typedef struct clit {
	struct clit previous;
	struct clit next;

	L4_ThreadId_t tid;
	void* table_entry;
} clock_item;


void clock_page_insert(clock_item*);
clock_item* clock_get_oldest(void);
clock_item* clock_remove_oldest(void);


#endif /* SWAPPER_H_ */
