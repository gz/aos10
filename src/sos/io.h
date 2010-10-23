#ifndef IO_H_
#define IO_H_

#include <l4/message.h>

#define MAX_FILES 16

typedef unsigned char* data_ptr;

/** Internal file handle representation */
struct file_table_entry {
	data_ptr buffer;
	L4_Word_t access_mode;
	L4_Word_t pos;
	L4_Word_t buffer_size;
	L4_ThreadId_t reader_tid;
};

typedef struct file_table_entry* file_handle;

/** File table */
void *file_table[MAX_FILES];

void open_file(L4_ThreadId_t tid, L4_Msg_t* msg_p);
void read_file(L4_ThreadId_t tid, L4_Msg_t* msg_p);
void write_file(L4_ThreadId_t tid, L4_Msg_t* msg_p);

#endif /* IO_H_ */
