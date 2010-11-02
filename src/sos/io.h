#ifndef IO_H_
#define IO_H_

#include <sos_shared.h>
#include <l4/message.h>
#include <l4/types.h>
#include "datastructures/circular_buffer.h"

struct fentry;
struct finfo;

/** Information we track for files in the file system (NFS and device files). */
typedef struct finfo {
	char filename[MAX_PATH_LENGTH]; /**< Buffer for the name of the file */
	int size;

	L4_ThreadId_t reader;							/**< for special files: thread who has read access*/
	struct serial* serial_handle;					/**< serial handler (only used for special files) */
	circular_buffer* cbuffer;

	int (*open)(struct finfo*, L4_ThreadId_t, L4_Msg_t*);
	int (*write)(struct fentry*, int, data_ptr);	/**< write function called for this file */
	void (*read)(struct fentry*);					/**< read function called for this file */
	void (*close)(struct fentry*);
} file_info;


/** Information we track for open files. */
typedef struct fentry {
	file_info* file;
	L4_ThreadId_t owner;
	fmode_t mode;

	data_ptr destination;		/**< pointer to user space memory location where we should write the data on read */
	L4_Word_t to_read;			/**< number of bytes to read (set by syscall read()) */
} file_table_entry;

#define DIR_CACHE_SIZE 0x100
extern file_info* file_cache[DIR_CACHE_SIZE];

void io_init(void);
int open_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int read_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int write_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int close_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int stat_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int get_dirent(L4_ThreadId_t, L4_Msg_t*, data_ptr);
fildes_t find_free_file_slot(file_table_entry**);

#endif /* IO_H_ */
