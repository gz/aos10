#ifndef IO_NFS_H_
#define IO_NFS_H_

#include <sos_shared.h>
#include <nfs.h>

void nfs_readdir_callback(uintptr_t, int, int, struct nfs_filename*, int);


#endif /* IO_NFS_H_ */
