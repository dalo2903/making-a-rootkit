
sudo insmod rootkit.ko
modinfo rootkit
dmesg
sudo rm /dev/rootkit
sudo mknod /dev/rootkit c 100 0
sudo chmod o+w /dev/rootkit
