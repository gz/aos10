#include <l4/types.h>
#include <l4/message.h>

void pager_init(void);
void pager(L4_ThreadId_t, L4_Msg_t*);
void pager_unmap_all(L4_ThreadId_t);
//void pager_set_access_rights(L4_Word_t, L4_Word_t, L4_Word_t);
