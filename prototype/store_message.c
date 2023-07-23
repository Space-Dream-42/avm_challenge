#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/fs.h>

MODULE_LICENSE("Dual BSD/GPL");
#define CDRV_MAJOR 42
#define BUF_LEN 256
#define CDRV_MAX_MINORS 1


struct cdrv_device_data {
    struct cdev cdev; // imported from cdev.h
    char buffer[BUF_LEN];
    size_t size;
    struct class*  cdrv_class; // imported from device.h
    struct device* cdrv_dev; // imported from device.h
};

struct cdrv_device_data char_device;

static int my_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "store_message: Device opened by userprogram\n");
    return 0;
}

static ssize_t my_write(struct file *file, const char __user *user_buffer, size_t count, loff_t * ppos)
{
    ssize_t bytes_written;

    if(count > BUF_LEN) {
        return -EINVAL;
    }

    // does return the number of bytes that could not be copied
    if(copy_from_user(char_device.buffer, user_buffer, count) != 0) {
        printk("Copying the data form user space to kernel space.\n");
        return -EFAULT;
    }

    bytes_written = count;
    char_device.size = bytes_written;
    return bytes_written;
}

static ssize_t my_read(struct file *file, char __user *user_buffer, size_t count, loff_t *ppos)
{
    ssize_t bytes_read;

    // Make sure our user isn't trying to read more data than there is.
    if(count > char_device.size) {
        count = char_device.size;
    }

    // If the position is behind the device's size, there is nothing to read.
    if (*ppos >= char_device.size) {
        return 0;
    }

    // Don't read across the buffer's end.
    if (*ppos + count > char_device.size) {
        count = char_device.size - *ppos;
    }

    // Copy data from the buffer to the user buffer.
    if(copy_to_user(user_buffer, char_device.buffer + *ppos, count) != 0) {
        return -EFAULT;
    }

    // Update our bookkeeping of where the file has been read up to.
    *ppos += count;
    bytes_read = count;

    return bytes_read;
}


const struct file_operations fops = {
    .read = my_read,
    .write = my_write,
    .open = my_open,
};

static int init(void)
{
    int count, ret_val;
    printk(KERN_ALERT "Initializing store_message driver...start\n");
    ret_val = register_chrdev_region(MKDEV(CDRV_MAJOR, 0), 
    1, "store_message_driver"); // register the character device driver. Look up: MKDEV, why is there a 0?
    // Error handling
    if (ret_val != 0) 
    {
        printk("register_chrdev_region():failed with error code:%d\n",ret_val);
        return ret_val;
    }
    // TO-DO: initialize the the driver. You don't need a for loop, we just want to create one driver
    cdev_init(&char_device.cdev, &fops);
    cdev_add(&char_device.cdev, MKDEV(CDRV_MAJOR, 0), 1);
    char_device.cdrv_class = class_create(THIS_MODULE, CDRV_CLASS_NAME);
    if (IS_ERR(char_device.cdrv_class)){
        printk(KERN_ALERT "cdrv : register device class failed\n");
        return PTR_ERR(char_device.cdrv_class);
    }
    char_device.size = BUF_LEN;
    printk(KERN_INFO "cdrv device class registered successfully\n");
    char_device.cdrv_dev = device_create(char_device.cdrv_class, NULL, MKDEV(CDRV_MAJOR, count), NULL, CDRV_DEVICE_NAME);

    return 0;
}

static void exit(void)
{
    // TO-Do: Unregister and destroy the device
    device_destroy(char_device.cdrv_class, &char_device.cdrv_dev);
    class_destroy(char_device.cdrv_class);
    cdev_del(&char_device.cdev);
    unregister_chrdev_region(MKDEV(CDRV_MAJOR, 0), CDRV_MAX_MINORS);
    printk(KERN_ALERT "Deloading store_message driver...done\n");
}



module_init(init);
module_exit(exit);