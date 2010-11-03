#ifndef IO_H_
#define IO_H_

#include <sos_shared.h>
#include <l4/message.h>
#include <l4/types.h>
#include "datastructures/circular_buffer.h"
#include <rpc.h>

struct fentry;
struct finfo;

/** Information we track for files in the file system (NFS and device files). */
typedef struct finfo {
	char filename[MAX_PATH_LENGTH]; /**< Buffer for the name of the file */
	stat_t status;

	L4_ThreadId_t reader;							/**< for special files: thread who has read access*/
	struct serial* serial_handle;					/**< serial handler (only used for special files) */
	circular_buffer* cbuffer;

	struct cookie nfs_handle;

	void (*open)  (struct finfo*, L4_ThreadId_t, fmode_t mode);
	void (*write) (struct fentry*);					/**< write function called for this file */
	void (*read)  (struct fentry*);					/**< read function called for this file  */
	void (*close) (struct fentry*);					/**< close function called for this file */
} file_info;


/** Information we track for open files. */
typedef struct fentry {
	file_info* file;
	L4_ThreadId_t owner;
	fmode_t mode;

	data_ptr client_buffer;		/**< pointer to user space memory location where we should write the data on read */
	L4_Word_t to_read;			/**< number of bytes to read (set by syscall read()) */
	L4_Word_t to_write;			/**< number of bytes to write (set by syscall write()) */

	L4_Word_t write_position;
	L4_Word_t read_position;

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
int file_cache_insert(file_info*);

#endif /* IO_H_ */
