/**
 * This header files contains definitions shared between the root
 * server and the user space library libsos.h
 */

#ifndef SOS_SHARED_H_
#define SOS_SHARED_H_

#include <stdlib.h>

#include "macros.h"
#include "syscalls.h"
#include "process_shared.h"
#include "io_shared.h"


#define IPC_MAX_WORDS 64

typedef char* data_ptr;

/* Predefined area in virtual address layout for IPC communication */
static const data_ptr ipc_memory_start = (data_ptr) 0x60000000;



#endif /* SOS_SHARED_H_ */
