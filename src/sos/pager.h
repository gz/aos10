#include <l4/types.h>
#include <l4/message.h>

extern void pager_init(void);

extern void pager(L4_ThreadId_t, L4_Msg_t*);
