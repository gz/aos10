#ifndef IO_NFS_H_
#define IO_NFS_H_

#include <sos_shared.h>
#include <nfs.h>
#include "io.h"

void nfs_readdir_callback(uintptr_t, int, int, struct nfs_filename*, int);

void open_nfs(file_info*, L4_ThreadId_t, fmode_t);
void read_nfs(file_table_entry* f);
void write_nfs(file_table_entry* f);
file_info* create_nfs(char* name, L4_ThreadId_t recipient, fmode_t mode);


#endif /* IO_NFS_H_ */
