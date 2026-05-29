KERNELRELEASE ?= $(shell uname -r)

obj-m += msiecmodule.o

all:
	$(MAKE) -C /lib/modules/$(KERNELRELEASE)/build M=$(CURDIR) modules

clean:
	$(MAKE) -C /lib/modules/$(KERNELRELEASE)/build M=$(CURDIR) clean
