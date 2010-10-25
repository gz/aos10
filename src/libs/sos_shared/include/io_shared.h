#ifndef IO_SHARED_H_
#define IO_SHARED_H_

typedef uint8_t fmode_t;
typedef uint8_t st_type_t;
typedef int fildes_t;

/* Limits */
#define PROCESS_MAX_FILES 16
#define MAX_IO_BUF 0x1000
#define MAX_PATH_LENGTH 255

/* file modes */
#define FM_WRITE 1
#define FM_READ  2
#define FM_EXEC  4

#define O_RDONLY FM_READ
#define O_WRONLY FM_WRITE
#define O_RDWR   (FM_READ|FM_WRITE)

/* stat file types */
#define ST_FILE 1	 /* plain file */
#define ST_SPECIAL 2	/* special (console) file */

typedef struct {
  st_type_t st_type;	/* file type */
  fmode_t   st_fmode;	/* access mode */
  size_t    st_size;	/* file size in bytes */
  long	    st_ctime;	/* file creation time (ms since booting) */
  long	    st_atime;	/* file last access (open) time (ms since booting) */
} stat_t;



#endif /* IO_SHARED_H_ */
