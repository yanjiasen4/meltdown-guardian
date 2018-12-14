KERN_VER = $(shell uname -r)
KERN_DIR = /lib/modules/$(KERN_VER)/build

obj-m += mg.o

all:
	make -C $(KERN_DIR) M=`pwd` modules

.PHONY: clean
clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
