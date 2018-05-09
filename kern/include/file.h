/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>
#define STARTING_INDEX 3

struct file {
	int mode_flag;
	struct vnode *f_vnode;
	struct lock *f_lock;						
	off_t f_offset;
	int f_refcount;				//use for garbage collection
};

struct file_table{
	struct file *files[OPEN_MAX];
};



//variable file length
int sys_open(char *filename,int flags,mode_t mode,int *file_pointer);

ssize_t sys_read(int fd, void *buf, size_t buflen, int32_t * retval);

ssize_t sys_write(int fd, const void *buf, size_t buflen, int * retval);

off_t sys_lseek(int fd, off_t pos, int whence, int *ret);

int sys_close(int fd, int *ret);

int sys_dup2(int oldfd, int newfd, int* ret);

void create_fileTable(void);
/*
 * Put your function declarations and data types here ...
 */



#endif /* _FILE_H_ */
