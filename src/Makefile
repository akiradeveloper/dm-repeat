obj-m := dm-repeat.o

KERNEL_TREE := /home/akiradeveloper/src/linux-3.1.5/
PWD := $(shell pwd)
EXTRA_CFLAGS='-Wall'

default:
	$(MAKE) -C $(KERNEL_TREE) M=$(PWD) -I $(KERNEL_TREE)/include -I modules

clean:
	rm -f *.o *.ko
