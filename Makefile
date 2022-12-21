ifneq ($(KERNELRELEASE),)
obj-m := mympu9250.o
else
KDIR := $(HOME)/IMD/buildroot/output/build/linux-5.15.35
all:
	$(MAKE) -C $(KDIR) M=$$PWD
endif
