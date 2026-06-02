obj-m += dht11_drv.o

# 百问 Buildroot 内核路径；若编译失败可改为 Linux-4.9.88 源码路径
KDIR ?= /home/book/100ask_imx6ull-sdk/Buildroot_2020.02.x/output/build/linux-custom
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	$(CROSS_COMPILE)gcc -o dht11_test dht11_test.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f dht11_test