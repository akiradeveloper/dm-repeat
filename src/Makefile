obj-m := dm-repeat.o

KERNEL_TREE := /home/akira/src/linux-3.1.5/
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNEL_TREE) M=$(PWD) modules

clean:
	rm -f *.o *.ko
