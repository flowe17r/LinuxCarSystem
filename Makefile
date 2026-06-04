obj-m += dht11_drv.o

KDIR ?= /home/book/100ask_imx6ull-sdk/Linux-4.9.88
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	$(CROSS_COMPILE)gcc -o dht11_test dht11_test.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f dht11_test
