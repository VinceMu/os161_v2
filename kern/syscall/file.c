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
   char* con1 = kstrdup("con:");
   char* con2 = kstrdup("con:"); //avoid overriding in memory
   struct vnode* node_out = NULL; 
   struct vnode* node_error = NULL;
   
   curthread->fileTable = (struct*file_table)kmalloc(sizeof(struct file_table));
   if(curthread->fileTable == NULL){
      //could not create file table due to not enough memory
      return ENOMEM;
   }
   //just for safety 
   for(int i=0;i<OPEN_MAX;i++){ //
     curthread->fileTable->file[i] = (struct *file)kmalloc(sizeof(struct file));
   }
   vfs_open(con1, O_RDONLY, 0, &node_out);
   vfs_open(con2,O_RDONLY,0,&node_err);
   if(node_out ==NULL || node_error ==NULL){
      //could not allocate node_out and node_error 
      return ENOMEM;
   }
   //ADD error check after vfs_open

   curthread->fileTable->file[1]->f_vnode = node_out;
   curthread->fileTable->file[1]-> mode_flag = O_WRONLY;
   curthread->fileTable->file[1]->f_offset = 0;
   curthread->fileTable->file[1]->f_refcount = 1;
   curthread->fileTable->file[1]->f_lock = lock_create("std_out");

   curthread->fileTable->file[2]->f_vnode = node_err;
   curthread->fileTable->file[2]-> mode_flag = O_WRONLY;
   curthread->fileTable->file[2]->f_offset = 0;
   curthread->fileTable->file[2]->f_refcount = 1;
   curthread->fileTable->file[1]->f_lock = lock_create("std_err");
   return;
 
}
