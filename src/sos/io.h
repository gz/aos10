#ifndef IO_H_
#define IO_H_

#include <sos_shared.h>
#include <l4/message.h>

struct fentry;
/** Information we track for open files. */
typedef struct fentry {
	char identifier[N_NAME];	/**< used to identify special files */
	L4_ThreadId_t owner;		/**< for special files: thread who has read access; file system: thread who opened the file */

	data_ptr buffer;			/**< buffer containing file content */
	int write_position;			/**< write position in buffer */
	int read_position;			/**< read position in buffer */

	data_ptr destination;		/**< pointer to user space memory location where we should write the data on read */

	L4_Bool_t reader_blocking;	/**< for special files, if client is atm waiting in read() call */
	L4_Bool_t double_overflow;	/**< signals that we have a overflow in the our buffer */
	L4_Word_t to_read;			/**< number of bytes to read (set by syscall read()) */

	struct serial* serial_handle;					/**< serial handler (only used for special files) */

	int (*write)(struct fentry*, int, data_ptr);	/**< write function called for this file */
	void (*read)(struct fentry*);					/**< read function called for this file */
} file_table_entry;

/** Information we track for files found in the NFS file system. */
typedef struct nfs_fentry {
	char filename[MAX_PATH_LENGTH]; /**< Buffer for the name of the file */
} nfs_file_table_entry;


#define SPECIAL_FILES 1
#define FILE_TABLE_ENTRIES SPECIAL_FILES
file_table_entry* file_table[FILE_TABLE_ENTRIES];


void io_init(void);
int open_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int read_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int write_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int close_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int get_dirent(L4_ThreadId_t, L4_Msg_t*, data_ptr);

#endif /* IO_H_ */
