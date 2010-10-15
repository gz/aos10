#include <l4/types.h>
#include <l4/message.h>

void pager_init(void);
void pager(L4_ThreadId_t, L4_Msg_t*);
void pager_unmap_all(void);
