/**
 * Identifiers for sos syscalls.
 */

#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#define GET_SYSCALL_NR(t)	((short) L4_Label(t) >> 4)
#define CREATE_SYSCALL_NR(t)	(t << 4)

/* Some IPC labels defined in the L4 documentation */
#define L4_PAGEFAULT	((L4_Word_t) -2)
#define L4_INTERRUPT	((L4_Word_t) -1)

#define SOS_OPEN 1
#define SOS_READ 2
#define SOS_WRITE 3

#define SOS_UNMAP_ALL 4



#endif /* SYSCALLS_H_ */
