#ifndef IO_SERIAL_H_
#define IO_SERIAL_H_

#include <serial.h>
#include "io.h"

struct serial* console_init(void);

void open_serial(file_info*, L4_ThreadId_t, fmode_t);
void read_serial(file_table_entry*);
void write_serial(file_table_entry*);
void close_serial(file_table_entry*);

void serial_receive_handler(struct serial*, char);

#endif /* IO_SERIAL_H_ */
