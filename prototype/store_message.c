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
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>
#include <linux/err.h>

MODULE_LICENSE("Dual BSD/GPL");
#define CDRV_MAJOR 42
#define WORD_LEN 64
#define CDRV_MAX_MINORS 1
#define NUM_OF_DEVICES 1
#define MAX_LIST_LEN 100
#define DEVICE_NAME "store_message"
#define TIMEOUT 1000


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
struct timer_list print_timer;
struct list_elem *elem_to_print;
static DEFINE_MUTEX(my_mutex);

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

    mutex_lock(&my_mutex);
    list_add_tail(&(new_elem->my_list), &(char_device.word_list_head.my_list));
    mutex_unlock(&my_mutex);

    bytes_written = count;
    char_device.list_len += 1;

    return bytes_written;
}

static ssize_t my_read(struct file *file, char __user *user_buffer, size_t count, loff_t *ppos)
{
    int words_read;
    int i;
    struct list_elem *current_elem;
    char *concatenated_words;
    int total_len = 0;
    char *space = ", ";

    concatenated_words = kmalloc(count * WORD_LEN + 3, GFP_KERNEL);
    if (!concatenated_words) {
        return -ENOMEM;
    }
    concatenated_words[0] = '\0'; 

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

    if(copy_to_user(user_buffer, concatenated_words, total_len + 1) != 0)
    {
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

void print_list_elem(struct timer_list *data)
{   
    printk("%s\n", elem_to_print->word);
    elem_to_print = list_entry((elem_to_print->my_list).next, struct list_elem, my_list);
    
    mod_timer(&print_timer, jiffies + msecs_to_jiffies(TIMEOUT));
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
    strncpy(char_device.word_list_head.word, "Start:", WORD_LEN);
    INIT_LIST_HEAD(&(char_device.word_list_head.my_list));
    char_device.list_len = 1;

    // Initialize the print timer
    timer_setup(&print_timer, print_list_elem, 0);
    print_timer.expires = jiffies + msecs_to_jiffies(TIMEOUT); 
    add_timer(&print_timer);
    elem_to_print = &(char_device.word_list_head);

    printk(KERN_ALERT "Initializing store_message driver...start\n");

    err = alloc_chrdev_region(&dev, 0, NUM_OF_DEVICES, DEVICE_NAME); // the major and minor will get stored in dev and will be dynamically produced by this function, and 0 is the starting number for the nummeration of the minors

    if(err < 0)
    {
        printk(KERN_ALERT "Failed to perform alloc_chrdev_region function.\n");
        return -1;
    }

    dev_major = MAJOR(dev);

    mychardev_class = class_create(THIS_MODULE, DEVICE_NAME); // creates sysfs class with paths for each character devices, this object can be carried to device_create. After the invocation you can see the devices under /dev/
    
    if(IS_ERR(mychardev_class))
    {
        printk(KERN_ALERT "Failed to perform class_create function.\n");
        return -1;
    }

    mychardev_class->dev_uevent = mychardev_uevent;

    cdev_init(&char_device.cdev, &fops);
    char_device.cdev.owner = THIS_MODULE;
    
    if(cdev_add(&char_device.cdev, MKDEV(dev_major, 0), 1) < 0)
    {
        printk(KERN_ALERT "Failed to perform cdev_add function.\n");
        return -1;
    }

    if(IS_ERR(device_create(mychardev_class, NULL, MKDEV(dev_major, 0), NULL, "store-message-dev-0")))
    {
        printk(KERN_ALERT "Failed to perform device_create function.\n");
        return -1;
    }

    return 0;
}

static void __exit my_exit(void)
{
    del_timer(&print_timer);
    if(IS_ERR(device_destroy(mychardev_class, MKDEV(dev_major, 0))))
    {
        printk(KERN_ALERT "Failed to perform device_destroy function.\n");
    }
    
    class_unregister(mychardev_class);
    class_destroy(mychardev_class);
    unregister_chrdev_region(MKDEV(dev_major, 0), MINORMASK);

    printk(KERN_ALERT "Deloading store_message driver...done\n");
}

module_init(my_init);
module_exit(my_exit);
