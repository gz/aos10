#include <l4/types.h>
#include <l4/message.h>
#include <sos_shared.h>

void pager_init(void);
void pager(L4_ThreadId_t, L4_Msg_t*);
int pager_unmap_all(L4_ThreadId_t tid, L4_Msg_t* msg_p, data_ptr buf);
void pager_free_all(L4_ThreadId_t);
void* pager_physical_lookup(L4_ThreadId_t, L4_Word_t addr);
