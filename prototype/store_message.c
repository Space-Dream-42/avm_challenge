#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>

MODULE_LICENSE("Dual BSD/GPL");
#define CDRV_MAJOR 42
#define WORD_LEN 64
#define CDRV_MAX_MINORS 1
#define NUM_OF_DEVICES 1
#define MAX_LIST_LEN 100
#define DEVICE_NAME "store_message"


struct list_elem
{
    struct list_head my_list;
    char word[WORD_LEN];
};

struct cdrv_device_data 
{
    struct cdev cdev; // imported from cdev.h
    struct list_elem word_list_head;
    int list_len;
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
    struct list_elem *new_elem;

    if (count > WORD_LEN)
    {
        printk(KERN_ALERT "Word is too long and cannot be inserted");
        return -EINVAL;
    }

    new_elem = (struct list_elem*) kmalloc(sizeof(struct list_elem), GFP_KERNEL);

    // does return the number of bytes that could not be copied
    if(copy_from_user(new_elem->word, user_buffer, count) != 0) {
        printk("Faild to copying all data form user space to kernel space.\n");
        return -EFAULT;
    }

    list_add_tail(&(new_elem->my_list), &(char_device.word_list_head.my_list));

    bytes_written = count;
    char_device.list_len += 1;

    return bytes_written;
}

static ssize_t my_read(struct file *file, char __user *user_buffer, size_t count, loff_t *ppos)
{
    // program a for loop which is looping count times
    int words_read;
    int i;
    struct list_elem *current_elem;
    char *concatenated_words;
    int total_len = 0;
    char *space = ", ";

    // Allocate memory for the concatenated words
    concatenated_words = kmalloc(count * WORD_LEN + 3, GFP_KERNEL);
    if (!concatenated_words) {
        return -ENOMEM;
    }
    concatenated_words[0] = '\0'; // Initialize the string

    // Make sure our user isn't trying to read more data than there is.
    if(count > char_device.list_len) {
        count = char_device.list_len;
    }

    for(i = 0; i < count + 1; i++)
    {
        if(i == 0)
        {
            current_elem = &char_device.word_list_head;
        }
        else
        {
            current_elem = list_entry((current_elem->my_list).next, struct list_elem, my_list);
        }

        strncat(concatenated_words, current_elem->word, WORD_LEN);
        
        if (i == 0 || i == count)
        {
            total_len += strlen(current_elem->word);
        }
        else
        {
            strncat(concatenated_words, space, 3);
            total_len += strlen(current_elem->word) + 3;
        }
        
    }
    words_read = count;

    if(copy_to_user(user_buffer, concatenated_words, total_len + 1) != 0) {
        kfree(concatenated_words);
        return -EFAULT;
    }

    kfree(concatenated_words);

    return words_read;
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

static int __init my_init(void)
{
    int err;
    dev_t dev; // This is 32 bit quantity with 12 bit for the major and 20 bit for the minor

    // Initialize the head of the list
    strncpy(char_device.word_list_head.word, ". Start:", WORD_LEN);
    INIT_LIST_HEAD(&(char_device.word_list_head.my_list));
    char_device.list_len = 1;

    // TO-DO: Error handling
    printk(KERN_ALERT "Initializing store_message driver...start\n");

    err = alloc_chrdev_region(&dev, 0, NUM_OF_DEVICES, DEVICE_NAME); // the major and minor will get stored in dev and will be dynamically produced by this function, and 0 is the starting number for the nummeration of the minors

    dev_major = MAJOR(dev);

    // sysfs is a virtual file system which is providing information about various kernel subsystems (like device drivers, or hardware devices)
    // sysfs can be found in /sys/
    mychardev_class = class_create(THIS_MODULE, DEVICE_NAME); // creates sysfs class with paths for each character devices, this object can be carried to device_create. After the invocation you can see the devices under /dev/
    mychardev_class->dev_uevent = mychardev_uevent;

    // Now it’s time to initialize a new character device and set file_operations with cdev_init.
    cdev_init(&char_device.cdev, &fops);
    char_device.cdev.owner = THIS_MODULE;
    
    // Inform the Kernel about your new device 
    cdev_add(&char_device.cdev, MKDEV(dev_major, 0), 1);


    device_create(mychardev_class, NULL, MKDEV(dev_major, 0), NULL, "store-message-dev-0");

    return 0;
}

static void __exit my_exit(void)
{
    // TO-DO: Error Handling
    device_destroy(mychardev_class, MKDEV(dev_major, 0));
    class_unregister(mychardev_class);
    class_destroy(mychardev_class);
    unregister_chrdev_region(MKDEV(dev_major, 0), MINORMASK);

    printk(KERN_ALERT "Deloading store_message driver...done\n");
}



module_init(my_init);
module_exit(my_exit);
