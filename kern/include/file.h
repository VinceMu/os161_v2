/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>

//variable file length
int open(const char *filename, int flags,...);

ssize_t read(int fd, void *buf, size_t buflen);

ssize_t write(int fd, const void *buf, size_t nbytes);

off_t lseek(int fd, off_t pos, int whence);

int close(int fd);

int dup2(int oldfd, int newfd);

/*
 * Put your function declarations and data types here ...
 */



#endif /* _FILE_H_ */
