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
#define NUM_OF_DEVICES 1
#define DEVICE_NAME "store_message"


struct cdrv_device_data {
    struct cdev cdev; // imported from cdev.h
    char buffer[BUF_LEN];
    size_t size;
    struct device* cdrv_dev; // imported from device.h
};

struct cdrv_device_data char_device;
static struct class *mychardev_class = NULL;
static int dev_major = 0;

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

    printk("Hello I am in the kernel mode and wrote things to the device.");
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

    printk("Hello I am in the kernel mode and read things to the device.");
    return bytes_read;
}

static int mychardev_uevent(struct device *dev, struct kobj_uevent_env *env)
{
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

const struct file_operations fops = {
    .read = my_read,
    .write = my_write,
    .open = my_open,
};

static int init(void)
{
    // TO-DO: Error handling

    int err;
    dev_t dev;
    printk(KERN_ALERT "Initializing store_message driver...start\n");

    err = alloc_chrdev_region(&dev, 0, NUM_OF_DEVICES, DEVICE_NAME); // the major will get stored in dev, and 0 is the first minor number

    dev_major = MAJOR(dev);

    // sysfs is a virtual file system which is providing information about various kernel subsystems (like device drivers, or hardware devices)
    // sysfs can be found in /sys/
    mychardev_class = class_create(THIS_MODULE, DEVICE_NAME); // creates sysfs class with paths for each character devices, this object can be carried to device_create. After the invocation you can see the devices under /dev/
    mychardev_class->dev_uevent = mychardev_uevent;

    // Now it’s time to initialize a new character device and set file_operations with cdev_init.
    cdev_init(&char_device.cdev, &fops);
    char_device.cdev.owner = THIS_MODULE;
    
    // Now add the device to the system.
    cdev_add(&char_device.cdev, MKDEV(dev_major, 0), 1);


    device_create(mychardev_class, NULL, MKDEV(dev_major, 0), NULL, "store-message-dev-0");

    return 0;
}

static void exit(void)
{
    // TO-DO: Error Handling
    device_destroy(mychardev_class, MKDEV(dev_major, 0));
    class_unregister(mychardev_class);
    class_destroy(mychardev_class);
    unregister_chrdev_region(MKDEV(dev_major, 0), MINORMASK);

    printk(KERN_ALERT "Deloading store_message driver...done\n");
}



module_init(init);
module_exit(exit);