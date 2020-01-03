# Making a Rootkit

## Background
### Loadable Kernel Module (LKM)
From [The Linux Kernel Module Programming Guide](http://www.tldp.org/LDP/lkmpg/2.6/html/index.html):
>Modules are pieces of code that can be loaded and unloaded into the kernel upon demand. They extend the functionality of the kernel without the need to reboot the system. For example, one type of module is the device driver, which allows the kernel to access hardware connected to the system. Without modules, we would have to build monolithic kernels and add new functionality directly into the kernel image. Besides having larger kernels, this has the disadvantage of requiring us to rebuild and reboot the kernel every time we want new functionality.

<p align="center">
  <img src="http://derekmolloy.ie/wp-content/uploads/2015/04/userspace-kernelspace.png"/>
  <br/>
</p>

### Device files
>Commonly, you can find device files in the /dev folder. They facilitate interaction between the user and the kernel code. If the kernel must receive anything, you can just write it to a device file to pass it to the module serving this file; anything that’s read from a device file originates from the module serving this file. 

## Reference
### Writing a Linux Kernel Module

* <http://derekmolloy.ie/writing-a-linux-kernel-module-part-1-introduction/>
* <http://derekmolloy.ie/writing-a-linux-kernel-module-part-2-a-character-device>
* [The Linux Kernel Module Programming Guide](http://www.tldp.org/LDP/lkmpg/2.6/html/index.html)
* https://www.apriorit.com/dev-blog/195-simple-driver-for-linux-os
### Linux Files
* https://www.codeproject.com/Articles/444995/Driver-to-hide-files-in-Linux-OS
* https://github.com/nurupo/rootkit/blob/master/rootkit.c
* https://ops.tips/amp/blog/how-is-proc-able-to-list-pids/

### Linux Devices
* https://www.debian.org/releases/jessie/amd64/apds01.html.en

### ioctl Interface
https://unix.stackexchange.com/questions/4711/what-is-the-difference-between-ioctl-unlocked-ioctl-and-compat-ioctl
### Linux Notification Chains
* https://0xax.gitbooks.io/linux-insides/content/Concepts/linux-cpu-4.html

### Keyboard
* https://elixir.bootlin.com/linux/v5.4.5/source/include/linux/keyboard.h

https://0x00sec.org/t/linux-keylogger-and-notification-chains/4566
