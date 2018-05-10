#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <kern/seek.h>
#include <lib.h>
#include <uio.h>
#include <thread.h>
#include <current.h>
#include <synch.h>
#include <vfs.h>
#include <vnode.h>
#include <file.h>
#include <syscall.h>
#include <copyinout.h>



int sys_open(char *filename,int flags,mode_t mode,int *file_pointer)
{
//   kprintf("started open\n");
   if(filename == NULL){
  //    kprintf("file name is null\n");
      return ENOENT;
   }
   struct vnode *file_node = NULL;
   //intialise file and get response code
   int file_index = STARTING_INDEX;
  // kprintf("before while loop\n");
   while(curthread->fileTable->files[file_index]->f_vnode != NULL){
    //  kprintf("incrementing file_index\n");
      file_index++;
   }
  // kprintf("file_index is %d \n",file_index);
   if(file_index >= OPEN_MAX){
      //file system is file.
      return EMFILE;
   }

   const int response = vfs_open(filename,flags,mode,&file_node);//
   //error checking
   if(response)
   {
   //   kprintf("received\n");
      return response;
   }
   curthread->fileTable->files[file_index]->mode_flag = flags;
   curthread->fileTable->files[file_index]->f_lock = lock_create(filename);
   curthread->fileTable->files[file_index]->f_vnode = file_node;
   curthread->fileTable->files[file_index]->f_refcount = 1; //should this be 2?
   curthread->fileTable->files[file_index]->f_offset = 0;

   *file_pointer = file_index;//return the index in the file table
   
   return 0;
}

int sys_close(int fd, int *retval)
{
   //kprintf("entered close\n");
   int flag = 0;
   if(fd < 0 || fd >= OPEN_MAX){
      return EBADF;
   }
   struct file* close_file = curthread->fileTable->files[fd];

   if(close_file == NULL) {
      return EBADF;
   }

   if(close_file->f_vnode == NULL) {
      return EBADF;
   }
   struct lock* lock_ptr = close_file->f_lock;//to release lock after closing.

   lock_acquire(close_file->f_lock);
   close_file->f_refcount--;

   if(close_file->f_refcount == 0){
      vfs_close(close_file->f_vnode);
      kfree(close_file);
      flag = 1;
   }else{
      curthread->fileTable->files[fd] = NULL;
   }

   lock_release(lock_ptr);

   if(flag == 1){
      lock_destroy(lock_ptr);
   }
//
   *retval = 0;
  // kprintf("exited close normally\n");
   return 0;
}

void create_fileTable(void){
   char* con1 = kstrdup("con:");
   char* con2 = kstrdup("con:"); //avoid overriding in memory
   struct vnode* node_out = NULL; 
   struct vnode* node_error = NULL;
   
   curthread->fileTable = (struct file_table *)kmalloc(sizeof(struct file_table));
   if(curthread->fileTable == NULL){
      //could not create file table due to not enough memory
      return;
   }
   //just for safety 
   char def_lock_name[] = "file_lock";
   for(int i=0;i<OPEN_MAX;i++){ 
     curthread->fileTable->files[i] = (struct file*)kmalloc(sizeof(struct file));
     curthread->fileTable->files[i]->f_lock = lock_create(def_lock_name);
     curthread->fileTable->files[i]->f_vnode = NULL;
   }
   vfs_open(con1, O_RDONLY, 0, &node_out);
   vfs_open(con2,O_RDONLY,0,&node_error);
   if(node_out ==NULL || node_error ==NULL){
      //could not allocate node_out and node_error 
      return;
   }
   //ADD error check after vfs_open

   curthread->fileTable->files[1]->f_vnode = node_out;
   curthread->fileTable->files[1]-> mode_flag = O_WRONLY;
   curthread->fileTable->files[1]->f_offset = 0;
   curthread->fileTable->files[1]->f_refcount = 1;
   curthread->fileTable->files[1]->f_lock = lock_create("std_out");

   curthread->fileTable->files[2]->f_vnode = node_error;
   curthread->fileTable->files[2]-> mode_flag = O_WRONLY;
   curthread->fileTable->files[2]->f_offset = 0;
   curthread->fileTable->files[2]->f_refcount = 1;
   curthread->fileTable->files[1]->f_lock = lock_create("std_err");
   return;
 
}

int sys_read(int fd, void *buf, size_t buflen, int32_t * retval)
{
   if(fd < 0 || fd >OPEN_MAX){
      *retval = -1;
      return EBADF;
   }
   struct iovec iov;
   struct uio read_io;
   struct file* open_file = curthread->fileTable->files[fd]; //get table 
 
   //disallow any concurrent modifications
   lock_acquire(open_file->f_lock); 

   if(open_file->mode_flag == O_WRONLY){
      //file is empty
      lock_release(open_file->f_lock);
      return EBADF;
   }
   char *char_buffer = (char*)kmalloc(buflen);//our char buffer which holds the characters. 
	   if(char_buffer == NULL) {
      lock_release(open_file->f_lock);
      kfree(char_buffer);
      return ENOMEM;
   }
   uio_kinit(&iov,&read_io,(void*)char_buffer,buflen,open_file->f_offset,UIO_READ);
   int response = VOP_READ(open_file->f_vnode,&read_io);

   if(response){
      lock_release(open_file->f_lock);
      kfree(char_buffer);
      return response;
   }
   open_file->f_offset = read_io.uio_offset;
   lock_release(open_file->f_lock);
   copyout((const void *)char_buffer, (userptr_t)buf, buflen); //copy out to void * buf
   kfree(char_buffer);
   *retval = buflen - read_io.uio_resid;//amount of bytes read.
   return 0;
}
int sys_write(int fd, const void *buf, size_t buflen, int * retval){
   if(fd < 0 || fd >= OPEN_MAX) return ENFILE;
   if(curthread->fileTable->files[fd] == NULL) return ENFILE;

   struct file* ofn = curthread->fileTable->files[fd];
   if(ofn->mode_flag == O_RDONLY) return EBADF;

   struct iovec io;
   struct uio kio;

   void * kbuf = kmalloc(sizeof(* buf) * buflen);
   if(kbuf == NULL) return EFAULT;

   copyin((const_userptr_t) buf, kbuf, buflen);

   io.iov_ubase = (userptr_t)buf;
   io.iov_len = buflen;

   lock_acquire(ofn->f_lock);
   kio.uio_iov = &io;
   kio.uio_iovcnt = 1;
   kio.uio_offset = ofn->f_offset;
   kio.uio_resid = buflen;
   kio.uio_segflg = UIO_USERSPACE;
   kio.uio_rw = UIO_WRITE;
   kio.uio_space = curthread->t_address_space;

   int ret;
   ret = VOP_WRITE(ofn->f_vnode, &kio);
   if(ret) {
      kfree(kbuf);
      lock_release(ofn->f_lock);
      * retval = -1;
      return ret;
   }

   ofn->f_offset = kio.uio_offset;
   lock_release(ofn->f_lock);
   * retval = buflen - kio.uio_resid;
   kfree(kbuf);
   return 0;
}

int sys_dup2(int oldfd, int newfd, int* retval){

   if (oldfd == newfd){
      *retval = newfd;
      return 0;
    }

   if (oldfd < 0 || oldfd >= OPEN_MAX){
      return EBADF;
   }

   if (newfd < 0 || newfd >= OPEN_MAX){
      return EBADF;
   }

   lock_acquire(curthread->fileTable->files[oldfd]->f_lock);
   curthread->fileTable->files[newfd]->mode_flag = curthread->fileTable->files[oldfd]->mode_flag;
   curthread->fileTable->files[newfd]->f_offset = curthread->fileTable->files[oldfd]->f_offset;
   curthread->fileTable->files[newfd]->f_vnode = curthread->fileTable->files[oldfd]->f_vnode;
   *retval = newfd;
   lock_release(curthread->fileTable->files[oldfd]->f_lock);
   return 0;
}

off_t sys_lseek(int fd, off_t pos, int whence, int64_t *retval)
{  
  // kprintf("started lseek\n");

   off_t offset;
   int result;
   struct stat f_info;

   if(fd >= OPEN_MAX || fd < 0){
      return EBADF;
   }

   struct file* seek_file = curthread->fileTable->files[fd];
   
   lock_acquire(seek_file->f_lock);
  // kprintf("lock_acquired\n");
   if(!VOP_ISSEEKABLE(seek_file->f_vnode)){
      return ESPIPE;
   }
   switch(whence){
      case SEEK_SET:
      offset = pos;
      break;

      case SEEK_CUR:
      offset = seek_file->f_offset + pos;
      break;

      case SEEK_END:
      result = VOP_STAT(seek_file->f_vnode, &f_info);//get info of file in this case the size 
      if(result){
         return result;
      }
      offset = f_info.st_size + pos; //as we reached the end move offset by file size
      break;

      default:
      lock_release(seek_file->f_lock);
      return EINVAL;
   }
   if(offset <(off_t)0){
      lock_release(seek_file->f_lock);
      return EINVAL;
   }
   seek_file->f_offset = offset;
  // int response = vop_tryseek(seek_file->f_vnode, offset);
   //if(response){
   //   *retval = -1;
   //}
   *retval = seek_file->f_offset = offset; 
   lock_release(seek_file->f_lock);
  // kprintf("retaval is %lld \n",*retval);
  // kprintf("exited lseek normally\n");
   return 0;
   
}


