obj-m += rootkit.o

all: client
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
client:
	gcc -o rk_client rk_client.c
test:	all
	./install_module.sh
	./remove_module.sh
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm rk_client
