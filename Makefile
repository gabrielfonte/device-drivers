obj-m := main.o

ARCH=arm
COMPILER=arm-linux-gnueabihf-
KERNEL_DIR=/home/$(USER)/Documentos/linux/
HOST_KERNEL_DIR=/lib/modules/6.7.6-arch1-2/build/

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(COMPILER) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(COMPILER) -C $(KERNEL_DIR) M=$(PWD) clean

help:
	make ARCH=$(ARCH) CROSS_COMPILE=$(COMPILER) -C $(KERNEL_DIR) M=$(PWD) help

host:
	make -C $(HOST_KERNEL_DIR) M=$(PWD) modules