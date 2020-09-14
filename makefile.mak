ifneq($(KERNELRELEASE),)
obj-m := hello.o
else
KDIR = /lib/modules/2.6.24.4/build
all:
	make -C $(KDIR) M = $(PWD)modules
clean:
	rm -f*.o *.ko *.mod.o *.order *.symvers
endif