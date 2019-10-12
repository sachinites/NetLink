BUILD_CFLAGS += -w
obj-m += NetlinkMultiCastLKM_recv1.o NetlinkMultiCastLKM_recv2.o
all:
	make -w -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
