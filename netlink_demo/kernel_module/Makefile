ifeq ($(KERNELRELEASE),)
TARGET := audit_mod
obj-m := $(TARGET).o
audit_mod-objs := stdhook.o syscalltable.o netlinkp.o
KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) modules
clean:
	rm -rf .*.cmd *.mod.c *.o *.ko .tmp* Module.*
install:
	insmod $(TARGET).ko
uninstall:
	rmmod $(TARGET)
else
obj-m := audit_mod.o
audit_mod-objs := stdhook.o syscalltable.o netlinkp.o
endif
