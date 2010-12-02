#ifndef IO_H_
#define IO_H_

#include <sos_shared.h>
#include <l4/message.h>
#include <l4/types.h>
#include <rpc.h>
#include "../datastructures/circular_buffer.h"

struct fentry;
struct finfo;

/** Information we track for files in the file system (NFS and device files). */
typedef struct finfo {
	char filename[MAX_PATH_LENGTH];					/**< Buffer for the name of the file */
	stat_t status;

	L4_Bool_t creation_pending;						/**< yet awaiting callback for file creation */
	L4_ThreadId_t reader;							/**< for special files: thread who has read access*/
	struct serial* serial_handle;					/**< serial handler (only used for special files) */
	circular_buffer* cbuffer;						/**< circular buffer (used for serial files) */

	struct cookie nfs_handle;						/**< handle used by NFS to identify the file */

	void (*open)  (struct finfo*, L4_ThreadId_t, fmode_t );
	void (*write) (struct fentry*);					/**< write function called for this file */
	void (*read)  (struct fentry*);					/**< read function called for this file  */
	void (*close) (struct fentry*);					/**< close function called for this file */

} file_info;

#define DIR_CACHE_SIZE 0x100
extern file_info* file_cache[DIR_CACHE_SIZE];	/**< used to store all file information */


/**
 * Information we track for open files.
 * These entries are stored in the process descriptor
 * (see process.h).
 **/
typedef struct fentry {
	file_info* file;			/**< pointer to the corresponding file_info */
	L4_ThreadId_t owner;		/**< owner of this file table entry */
	fmode_t mode;				/**< mode in which the file was opened */
	L4_Bool_t awaits_callback; /**< used for process deletion to determine what to do with this handle */

	data_ptr client_buffer;		/**< pointer to user space memory location where we should write the data on read */
	L4_Word_t to_read;			/**< number of bytes to read (set by syscall read()) */
	L4_Word_t to_write;			/**< number of bytes to write (set by syscall write()) */

	L4_Word_t write_position;	/**< current write position in file (to handle multiple write calls) */
	L4_Word_t read_position;	/**< current read position in file (to handle multiple read calls) */

} file_table_entry;


void io_init(void);
int open_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int read_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int write_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int close_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int stat_file(L4_ThreadId_t, L4_Msg_t*, data_ptr);
int get_dirent(L4_ThreadId_t, L4_Msg_t*, data_ptr);
fildes_t find_free_file_slot(file_table_entry**);
int file_cache_insert(file_info*);
file_table_entry* create_file_descriptor(file_info*, L4_ThreadId_t, fmode_t);

#endif /* IO_H_ */
