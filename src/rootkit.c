#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/fs.h>
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
  // sprintf(msg, "What is in the buffer: %s\n", msg);
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
    /* 
     * This ioctl is both input (ioctl_param) and 
     * output (the return value of this function) 
     */
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

// Rootkit init & exit
static int __init rootkit_init(void)
{
  int ret_val;
  
  /* 
   * Register the character device (atleast try) 
   */
  ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops);

  /* 
   * Negative values signify an error 
   */
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
  printk(KERN_INFO "The device file name is important, because\n");
  printk(KERN_INFO "the ioctl program assumes that's the\n");
  printk(KERN_INFO "file you'll use.\n");
  sprintf(msg, "Starting text\n");
  return 0;
}
static void __exit rootkit_exit(void)
{
  /*int ret =*/ unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
  //if (ret < 0)
  //    	printk(KERN_ALERT "Error in unregister_chrdev: %d\n", ret);
  printk(KERN_INFO "Rootkit unloaded\n");
}

module_init(rootkit_init);
module_exit(rootkit_exit);
