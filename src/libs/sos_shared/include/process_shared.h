#ifndef PROCESS_SHARED_H_
#define PROCESS_SHARED_H_

#define N_NAME 32

typedef int pid_t;

typedef struct {
  pid_t     pid;
  unsigned  size;		/* in pages */
  unsigned  stime;		/* start time in msec since booting */
  unsigned  ctime;		/* CPU time accumulated in msec */
  char	    command[N_NAME];	/* Name of exectuable */
} process_t;

#endif /* PROCESS_SHARED_H_ */
