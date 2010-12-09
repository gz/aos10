#ifndef PAGER_H_
#define PAGER_H_

#include <l4/types.h>
#include <l4/message.h>
#include <sos_shared.h>
#include "../queue.h"

/** List element used in the first level page table */
typedef struct page_entry {
	union {
		L4_Word_t address;
		void* address_ptr;
	};
} page_table_entry;

// Virtual address space layout constants
#define ONE_MEGABYTE (1024*1024)
#define VIRTUAL_START 0x2000000

//#define ELF_START VIRTUAL_START
//#define ELF_END (VIRTUAL_START + 8*ONE_MEGABYTE)

#define TEXT_START VIRTUAL_START
#define TEXT_END (VIRTUAL_START + 4*ONE_MEGABYTE)

#define DATA_START TEXT_END
#define DATA_END (DATA_START + 4*ONE_MEGABYTE)

#define STACK_TOP 0xC0000000
#define STACK_END (STACK_TOP - ONE_MEGABYTE)

#define IPC_START 0x60000000
#define IPC_END (IPC_START + 4096)

#define HEAP_START 0x40000000
#define HEAP_END (HEAP_START + (4 * ONE_MEGABYTE))

// Page table manipulation macros and constants
#define FIRST_LEVEL_BITS 12
#define FIRST_LEVEL_ENTRIES (1 << FIRST_LEVEL_BITS)
#define SECOND_LEVEL_BITS 8
#define SECOND_LEVEL_ENTRIES (1 << SECOND_LEVEL_BITS)

void pager_init(void);
int pager(L4_ThreadId_t, L4_Msg_t*);
int pager_unmap_all(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf);
void pager_free_all(L4_ThreadId_t);
void* pager_physical_lookup(L4_ThreadId_t, L4_Word_t addr);
page_table_entry* pager_table_lookup(L4_ThreadId_t, L4_Word_t);

#define CLEAR_LOWER_BITS(addr) ((addr) & ~0xFFF)

#endif /* PAGER_H_ */
