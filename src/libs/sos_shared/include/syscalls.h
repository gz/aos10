/**
 * Identifiers for sos syscalls.
 */

#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#define GET_SYSCALL_NR(t)	((short) L4_Label(t) >> 4)
#define CREATE_SYSCALL_NR(t)	(t << 4)

// Some IPC labels defined in the L4 documentation
#define L4_PAGEFAULT	((L4_Word_t) -2)
#define L4_INTERRUPT	((L4_Word_t) -1)

// IO Syscall Labels
#define SOS_OPEN 			 0
#define SOS_READ 			 1
#define SOS_WRITE 			 2
#define SOS_CLOSE 			 3
#define SOS_GETDIRENT 		 4
#define SOS_STAT 			 5

// Process Syscall Labels
#define SOS_PROCESS_CREATE 	 6
#define SOS_PROCESS_DELETE 	 7
#define SOS_PROCESS_ID 		 8
#define SOS_PROCESS_STATUS 	 9
#define SOS_PROCESS_WAIT 	10

// IO Syscall Labels
#define SOS_SLEEP 			11
#define SOS_TIMESTAMP 		12

// Debug Syscall Labels
#define SOS_UNMAP_ALL		13

#endif /* SYSCALLS_H_ */
