/*
 * DHT11 温湿度传感器驱动（100ASK IMX6ULL Pro）
 * 设备节点: /dev/mydht11
 * read() 返回 4 字节: [湿度整数, 湿度小数, 温度整数, 温度小数]
 * 需在设备树中添加 compatible = "100ask,dht11", gpios = <&gpio4 19 GPIO_ACTIVE_HIGH>
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/irqflags.h>
#include <linux/slab.h>

static int major;
static struct class *dht11_class;
static struct gpio_desc *dht11_data_pin;

static void dht11_reset(void)
{
	gpiod_direction_output(dht11_data_pin, 1);
}

static void dht11_start(void)
{
	mdelay(30);
	gpiod_set_value(dht11_data_pin, 0);
	mdelay(20);
	gpiod_set_value(dht11_data_pin, 1);
	udelay(40);
	gpiod_direction_input(dht11_data_pin);
	udelay(2);
}

static int dht11_wait_for_ready(void)
{
	int timeout_us = 20000;

	while (gpiod_get_value(dht11_data_pin) && --timeout_us)
		udelay(1);
	if (!timeout_us)
		return -1;

	timeout_us = 200;
	while (!gpiod_get_value(dht11_data_pin) && --timeout_us)
		udelay(1);
	if (!timeout_us)
		return -1;

	timeout_us = 200;
	while (gpiod_get_value(dht11_data_pin) && --timeout_us)
		udelay(1);
	if (!timeout_us)
		return -1;

	return 0;
}

static int dht11_read_byte(unsigned char *buf)
{
	int i;
	int us;
	unsigned char data = 0;
	int timeout_us;

	for (i = 0; i < 8; i++) {
		timeout_us = 400;
		while (!gpiod_get_value(dht11_data_pin) && --timeout_us)
			udelay(1);
		if (!timeout_us)
			return -1;

		udelay(40);

		if (gpiod_get_value(dht11_data_pin)) {
			data = (data << 1) | 1;
			timeout_us = 400;
			while (gpiod_get_value(dht11_data_pin) && --timeout_us)
				udelay(1);
			if (!timeout_us)
				return -1;
		} else {
			data = (data << 1) | 0;
		}
	}

	*buf = data;
	return 0;
}

static ssize_t dht11_drv_read(struct file *file, char __user *buf,
			      size_t size, loff_t *offset)
{
	unsigned long flags;
	int i;
	unsigned char data[5];

	if (size != 4)
		return -EINVAL;

	local_irq_save(flags);

	dht11_reset();
	dht11_start();

	if (dht11_wait_for_ready()) {
		local_irq_restore(flags);
		return -EAGAIN;
	}

	for (i = 0; i < 5; i++) {
		if (dht11_read_byte(&data[i])) {
			local_irq_restore(flags);
			return -EAGAIN;
		}
	}

	dht11_reset();
	local_irq_restore(flags);

	if (data[4] != (data[0] + data[1] + data[2] + data[3])) {
		pr_err("dht11: checksum error\n");
		return -EIO;
	}

	if (copy_to_user(buf, data, 4))
		return -EFAULT;

	return 4;
}

static struct file_operations dht11_fops = {
	.owner = THIS_MODULE,
	.read  = dht11_drv_read,
};

static int dht11_probe(struct platform_device *pdev)
{
	dht11_data_pin = gpiod_get(&pdev->dev, NULL, GPIOD_OUT_HIGH);
	if (IS_ERR(dht11_data_pin)) {
		pr_err("dht11: failed to get gpio\n");
		return PTR_ERR(dht11_data_pin);
	}

	device_create(dht11_class, NULL, MKDEV(major, 0), NULL, "mydht11");
	return 0;
}

static int dht11_remove(struct platform_device *pdev)
{
	device_destroy(dht11_class, MKDEV(major, 0));
	gpiod_put(dht11_data_pin);
	return 0;
}

static const struct of_device_id ask100_dht11[] = {
	{ .compatible = "100ask,dht11" },
	{ },
};
MODULE_DEVICE_TABLE(of, ask100_dht11);

static struct platform_driver dht11_driver = {
	.probe  = dht11_probe,
	.remove = dht11_remove,
	.driver = {
		.name           = "100ask_dht11",
		.of_match_table = ask100_dht11,
	},
};

static int __init dht11_init(void)
{
	int err;

	major = register_chrdev(0, "dht11", &dht11_fops);
	if (major < 0)
		return major;

	dht11_class = class_create(THIS_MODULE, "dht11_class");
	if (IS_ERR(dht11_class)) {
		unregister_chrdev(major, "dht11");
		return PTR_ERR(dht11_class);
	}

	err = platform_driver_register(&dht11_driver);
	if (err) {
		class_destroy(dht11_class);
		unregister_chrdev(major, "dht11");
	}

	return err;
}

static void __exit dht11_exit(void)
{
	platform_driver_unregister(&dht11_driver);
	class_destroy(dht11_class);
	unregister_chrdev(major, "dht11");
}

module_init(dht11_init);
module_exit(dht11_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Smart_car");
MODULE_DESCRIPTION("DHT11 temperature/humidity driver for 100ASK IMX6ULL Pro");
