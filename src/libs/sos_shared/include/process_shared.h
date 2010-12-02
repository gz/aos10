#ifndef PROCESS_SHARED_H_
#define PROCESS_SHARED_H_

#define N_NAME 32

// Error returns for process_create syscall (server side)
#define EXECUTABLE_NOT_FOUND -1
#define PROCESS_TABLE_FULL -2

typedef int pid_t;

typedef struct {
  pid_t     pid;
  unsigned  size;		/* in pages */
  unsigned  stime;		/* start time in msec since booting */
  unsigned  ctime;		/* CPU time accumulated in msec */
  char	    command[N_NAME];	/* Name of exectuable */
} process_t;

#endif /* PROCESS_SHARED_H_ */
