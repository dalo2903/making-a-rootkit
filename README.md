# Making a Rootkit

## Background
### Loadable Kernel Module (LKM)
From [The Linux Kernel Module Programming Guide](http://www.tldp.org/LDP/lkmpg/2.6/html/index.html):
>Modules are pieces of code that can be loaded and unloaded into the kernel upon demand. They extend the functionality of the kernel without the need to reboot the system. For example, one type of module is the device driver, which allows the kernel to access hardware connected to the system. Without modules, we would have to build monolithic kernels and add new functionality directly into the kernel image. Besides having larger kernels, this has the disadvantage of requiring us to rebuild and reboot the kernel every time we want new functionality.

The LKM is ideal for 


<p align="center">
  <img src="http://derekmolloy.ie/wp-content/uploads/2015/04/userspace-kernelspace.png"/>
  <br/>
</p>

## Reference
### Writing a Linux Kernel Module

* <http://derekmolloy.ie/writing-a-linux-kernel-module-part-1-introduction/>
* <http://derekmolloy.ie/writing-a-linux-kernel-module-part-2-a-character-device>
* [The Linux Kernel Module Programming Guide](http://www.tldp.org/LDP/lkmpg/2.6/html/index.html)

### Linux Devices
* https://www.debian.org/releases/jessie/amd64/apds01.html.en

### Linux Notification Chains
* https://0xax.gitbooks.io/linux-insides/content/Concepts/linux-cpu-4.html

### Keyboard
* https://elixir.bootlin.com/linux/v5.4.5/source/include/linux/keyboard.h

https://0x00sec.org/t/linux-keylogger-and-notification-chains/4566
