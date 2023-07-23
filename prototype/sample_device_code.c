#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>

#define CDRV_MAJOR       42
#define CDRV_MAX_MINORS  1
#define BUF_LEN 256
#define CDRV_DEVICE_NAME "cdrv_dev"
#define CDRV_CLASS_NAME "cdrv_class"

struct cdrv_device_data {
    struct cdev cdev;
    char buffer[BUF_LEN];
    size_t size;
    struct class*  cdrv_class;
    struct device* cdrv_dev;
};

struct cdrv_device_data char_device[CDRV_MAX_MINORS];
static ssize_t cdrv_write(struct file *file, const char __user *user_buffer,
                    size_t size, loff_t * offset)
{
    struct cdrv_device_data *cdrv_data = &char_device[0];
    ssize_t len = min(cdrv_data->size - *offset, size);
    printk("writing:bytes=%d\n",size);
    if (len buffer + *offset, user_buffer, len))
        return -EFAULT;

    *offset += len;
    return len;
}

static ssize_t cdrv_read(struct file *file, char __user *user_buffer,
                   size_t size, loff_t *offset)
{
    struct cdrv_device_data *cdrv_data = &char_device[0];
    ssize_t len = min(cdrv_data->size - *offset, size);

    if (len buffer + *offset, len))
        return -EFAULT;

    *offset += len;
    printk("read:bytes=%d\n",size);
    return len;
}
static int cdrv_open(struct inode *inode, struct file *file){
   printk(KERN_INFO "cdrv: Device open\n");
   return 0;
}

static int cdrv_release(struct inode *inode, struct file *file){
   printk(KERN_INFO "cdrv: Device closed\n");
   return 0;
}

const struct file_operations cdrv_fops = {
    .owner = THIS_MODULE,
    .open = cdrv_open,
    .read = cdrv_read,
    .write = cdrv_write,
    .release = cdrv_release,
};
int init_cdrv(void)
{
    int count, ret_val;
    printk("Init the basic character driver...start\n");
    ret_val = register_chrdev_region(MKDEV(CDRV_MAJOR, 0), CDRV_MAX_MINORS,
                                 "cdrv_device_driver");
    if (ret_val != 0) {
        printk("register_chrdev_region():failed with error code:%d\n",ret_val);
        return ret_val;
    }

    for(count = 0; count < CDRV_MAX_MINORS; count++) {
        cdev_init(&char_device[count].cdev, &cdrv_fops);
        cdev_add(&char_device[count].cdev, MKDEV(CDRV_MAJOR, count), 1);
        char_device[count].cdrv_class = class_create(THIS_MODULE, CDRV_CLASS_NAME);
        if (IS_ERR(char_device[count].cdrv_class)){
             printk(KERN_ALERT "cdrv : register device class failed\n");
             return PTR_ERR(char_device[count].cdrv_class);
        }
        char_device[count].size = BUF_LEN;
        printk(KERN_INFO "cdrv device class registered successfully\n");
        char_device[count].cdrv_dev = device_create(char_device[count].cdrv_class, NULL, MKDEV(CDRV_MAJOR, count), NULL, CDRV_DEVICE_NAME);

    }

    return 0;
}

void cleanup_cdrv(void)
{
    int count;

    for(count = 0; count < CDRV_MAX_MINORS; count++) {
        device_destroy(char_device[count].cdrv_class, &char_device[count].cdrv_dev);
        class_destroy(char_device[count].cdrv_class);
        cdev_del(&char_device[count].cdev);
    }
    unregister_chrdev_region(MKDEV(CDRV_MAJOR, 0), CDRV_MAX_MINORS);
    printk("Exiting the basic character driver...\n");
}
module_init(init_cdrv);
module_exit(cleanup_cdrv);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sushil Rathore");
MODULE_DESCRIPTION("Sample Character Driver");
MODULE_VERSION("1.0");