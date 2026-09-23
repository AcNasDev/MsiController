KERNELRELEASE ?= $(shell uname -r)
KERNEL_BUILD_DIR := /lib/modules/$(KERNELRELEASE)/build
STRUCTLEAK_PLUGIN := $(KERNEL_BUILD_DIR)/scripts/gcc-plugins/structleak_plugin.so

obj-m += msiecmodule.o

# Old kernel headers can ship a GCC plugin built for a different GCC ABI.
# Bypass an incompatible structleak plugin only when no other GCC plugin is
# enabled. RANDSTRUCT in particular changes structure layouts and is required
# for a module to match the running kernel.
ifeq ($(KBUILD_EXTMOD),)
ifneq ($(wildcard $(STRUCTLEAK_PLUGIN)),)
ifneq ($(shell printf '' | $(CC) -fplugin=$(STRUCTLEAK_PLUGIN) -x c -c -o /dev/null - >/dev/null 2>&1 && echo yes),yes)
ifeq ($(wildcard $(KERNEL_BUILD_DIR)/.config),)
$(error Kernel config is required to check GCC plugin compatibility)
endif
ifneq ($(shell grep -s '^CONFIG_GCC_PLUGIN_.*=y' $(KERNEL_BUILD_DIR)/.config | grep -v '^CONFIG_GCC_PLUGIN_STRUCTLEAK'),)
$(error Kernel GCC plugins require the compiler used to build the kernel)
endif
KBUILD_EXTRA_ARGS := GCC_PLUGINS_CFLAGS=
$(info msiecmodule: bypassing an incompatible structleak GCC plugin)
endif
endif
endif

all:
	$(MAKE) -C $(KERNEL_BUILD_DIR) M=$(CURDIR) $(KBUILD_EXTRA_ARGS) modules

clean:
	$(MAKE) -C $(KERNEL_BUILD_DIR) M=$(CURDIR) clean
