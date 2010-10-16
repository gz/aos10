/*
 * syscalls.h
 *
 *  Created on: Sep 30, 2010
 *      Author: gz
 */

#ifndef SYSCALLS_H_
#define SYSCALLS_H_

/* Some IPC labels defined in the L4 documentation */
#define L4_PAGEFAULT	((L4_Word_t) -2)
#define L4_INTERRUPT	((L4_Word_t) -1)


#define SOS_SERIAL_WRITE 1
#define SOS_UNMAP_ALL 2


#endif /* SYSCALLS_H_ */
