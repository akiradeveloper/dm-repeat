obj-m := dm-repeat.o

KERNEL_TREE := /home/akira/src/linux
PWD := $(shell pwd)
EXTRA_CFLAGS='-Wall'

default:
	$(MAKE) -C $(KERNEL_TREE) M=$(PWD) -I $(KERNEL_TREE)/include -I modules

clean:
	rm -f *.o *.ko
