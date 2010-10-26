#ifndef SYSENT_H_
#define SYSENT_H_

#include <l4/types.h>
#include <l4/ipc.h>

typedef int(*syscall_function_ptr)(L4_ThreadId_t, L4_Msg_t*, data_ptr);

#define SYSENT_SIZE 16
syscall_function_ptr sysent[SYSENT_SIZE];

void init_systable(void);

#endif /* SYSENT_H_ */
