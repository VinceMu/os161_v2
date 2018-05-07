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

/*
 * Add your file-related functions here ...
 */

int open(const char *filename,int flags,mode_t mode,int *)
{
   struct vnode *file_node; 	
   //intialise file and get response code
   int response = vfs_open(filename,flags,mode,&file_node);//
   if(response)
   {
      return response;
   }
   struct lock *file_lock = lock_create("file_open_lock"); 
   if(struct file_lock == NULL)//could not create lock 
   {
      vfs_close(file_node);
      return ENOMEM; //return not enough memory error 
   }
   /**
   not finished!!!!
   */
   return 0;
} 
void create_fileTable(){
     
}
