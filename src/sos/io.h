#ifndef IO_H_
#define IO_H_

#include <sos_shared.h>
#include <l4/message.h>

/** Internal file handle representation */
struct fentry;
typedef struct fentry {
	char identifier[N_NAME];

	data_ptr buffer;
	int position;

	data_ptr destination;
	L4_ThreadId_t reader_tid;
	L4_Bool_t reader_blocking;

	struct serial* serial_handle;
	int (*write)(struct fentry*, int, data_ptr);
	int (*read)(struct fentry*, int, data_ptr);
} file_table_entry;

#define SPECIAL_FILES 1
file_table_entry special_table[SPECIAL_FILES];


void io_init(void);
void open_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int read_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);
void write_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);

#endif /* IO_H_ */
