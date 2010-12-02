#ifndef _CLOCK_H_
#define _CLOCK_H_

#include <l4/types.h>
#include <l4/ipc.h>
#include <stdint.h>

/** Return codes for driver functions */
#define CLOCK_R_OK     0	/* success  */
#define CLOCK_R_UINT (-1)	/* driver not initialised */
#define CLOCK_R_CNCL (-2)	/* operation cancelled (driver stopped) */
#define CLOCK_R_FAIL (-3)	/* operation failed for other reason */

typedef uint64_t timestamp_t;

struct al;
typedef void (*alarm_function)(L4_ThreadId_t, int);

/** Timer Queue elements */
typedef struct al {
  struct al  *next_alarm;			/**< next alarm in queue */
  timestamp_t expiration_time;		/**< time in microseconds since booting when this timer should ring */
  alarm_function alarm_function;	/**< function to be called when we reached expiration time */
  L4_ThreadId_t owner;				/**< the client who "owns" this timer */
} alarm_timer;


/*
 * Initialise driver. Performs implicit stop_timer() if already initialised.
 *
 * Returns CLOCK_R_OK iff successful.
 */
int start_timer(void); 

/*
 * Register a client thread to be woken up by an IPC.
 *    delay:  Delay time in microseconds before sending wakeup IPC to client.
 *    client: Client id.
 *
 * Returns CLOCK_R_OK iff successful.
 */
int register_timer(uint64_t delay, L4_ThreadId_t client);  

/*
 * Removes all currently registred timers for a given thread.
 *    tid:    Thread ID.
 *
 */
void remove_timers(L4_ThreadId_t tid);

/*
 * Returns present time in microseconds since booting.
 *
 * Returns a negative value if failure.
 */
timestamp_t time_stamp(void);

/*
 * Stop clock driver operation.
 *
 * Returns CLOCK_R_OK iff successful.
 */
int stop_timer(void);


void timer_queue_insert(alarm_timer*);
alarm_timer* timer_queue_pop(void);

int timer_overflow_irq(L4_ThreadId_t, L4_Msg_t*);
int timer0_irq(L4_ThreadId_t, L4_Msg_t*);


int sleep_timer(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int send_timestamp(L4_ThreadId_t, L4_Msg_t*, data_ptr);

//void test_the_clock(void);

#endif /* _CLOCK_H_ */
