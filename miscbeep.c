#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/types.h>
#include <linux/ide.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>

#define DEVICE_MINOR    100
#define BEEP_ON         1
#define BEEP_OFF        0

int miscbeep_probe(struct platform_device *dev);
int miscbeep_remove(struct platform_device *dev);
static int miscbeep_open(struct inode *node, struct file *filp);
static ssize_t miscbeep_write(struct file *filp,                                                                                                                                             
                         const char __user *user,
                         size_t size,
                         loff_t *loffp);
static int miscbeep_release(struct inode *node, struct file *filp);

static const struct of_device_id of_miscbeep_match[] = {
        { .compatible = "alientek, beep", },
        { /* Sentinel */ },
};

static struct platform_driver miscbeep_driver = {
        .driver = {
                .name = "miscbeep",
                .of_match_table = of_miscbeep_match,
        },
        .probe = miscbeep_probe,
        .remove = miscbeep_remove,
};

struct miscbeep_dev_struct {
        struct device_node *node;
        int beep_gpio;
};
static struct miscbeep_dev_struct miscbeep_dev;

static struct file_operations miscbeep_fops = {
        .owner = THIS_MODULE,
        .open = miscbeep_open,
        .write = miscbeep_write,
        .release = miscbeep_release,
};

static struct miscdevice miscdevice_beep = {
        .minor = DEVICE_MINOR,
        .name = "misc_beep",
        .fops = &miscbeep_fops,
};

static int miscbeep_open(struct inode *node, struct file *filp)
{
        filp->private_data = &miscbeep_dev;
        return 0;
}
static ssize_t miscbeep_write(struct file *filp,                                                                                                                                             
                         const char __user *user,
                         size_t size,
                         loff_t *loffp)
{
        int ret = 0;
        unsigned char buf[1] = {0};
        struct miscbeep_dev_struct *dev;

        dev = filp->private_data;
        
        ret = copy_from_user(buf, user, 1);
        if (ret < 0) {
                printk("kernel write error!\n");
                ret = -EINVAL;
                goto error;
        }

        switch (buf[0]) {
        case BEEP_ON:
                gpio_set_value(dev->beep_gpio, 0);
                break;
        case BEEP_OFF:
                gpio_set_value(dev->beep_gpio, 1);
                break;
        default:
                ret = -EINVAL;
                break;
        }

error:
        return ret;
}

static int miscbeep_release(struct inode *node, struct file *filp)
{
        filp->private_data = NULL;
        return 0;
}

int miscbeep_probe(struct platform_device *dev)
{
        int ret = 0;
        miscbeep_dev.node = dev->dev.of_node;
        if (miscbeep_dev.node == NULL) {
                ret = -EINVAL;
                goto fail_node;
        }

        miscbeep_dev.beep_gpio = of_get_named_gpio(miscbeep_dev.node,
                                                "beep-gpios", 0);
        if (miscbeep_dev.beep_gpio < 0) {
                ret = -EINVAL;
                goto fail_get_named;
        }
                                                
        ret = gpio_request(miscbeep_dev.beep_gpio, "miscbeep");
        if (ret) {
                ret = -EINVAL;
                goto fail_request;
        }

        ret = gpio_direction_output(miscbeep_dev.beep_gpio, 1);
        if (ret) {
                ret = -EINVAL;
                goto fail_dir_set;
        }

        misc_register(&miscdevice_beep);

        goto success;

fail_node:
fail_get_named:
fail_request:
fail_dir_set:
        gpio_free(miscbeep_dev.beep_gpio);
success:
        return 0;
}

int miscbeep_remove(struct platform_device *dev)
{
        gpio_set_value(miscbeep_dev.beep_gpio, 1);
        gpio_free(miscbeep_dev.beep_gpio);
        misc_deregister(&miscdevice_beep);
        return 0;
}

static int __init miscbeep_init(void)
{
        return platform_driver_register(&miscbeep_driver);
}

static void __exit miscbeep_exit(void)
{
        platform_driver_unregister(&miscbeep_driver);
}

module_init(miscbeep_init);
module_exit(miscbeep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("wanglei");