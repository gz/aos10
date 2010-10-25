#ifndef IO_H_
#define IO_H_

#include <sos_shared.h>
#include <l4/message.h>

/** Internal file handle representation */
struct fentry;
typedef struct fentry {
	char identifier[N_NAME];
	L4_ThreadId_t owner;

	data_ptr buffer;
	int write_position;
	int read_position;

	data_ptr destination;

	L4_Bool_t reader_blocking;
	L4_Bool_t double_overflow;
	L4_Word_t to_read;

	struct serial* serial_handle;
	int (*write)(struct fentry*, int, data_ptr);
	void (*read)(struct fentry*);
} file_table_entry;

#define SPECIAL_FILES 1
#define FILE_TABLE_ENTRIES SPECIAL_FILES
file_table_entry* file_table[FILE_TABLE_ENTRIES];


void io_init(void);
int open_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int read_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int write_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int close_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);

#endif /* IO_H_ */
