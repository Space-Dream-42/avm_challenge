#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/timer.h>

struct word_node {
    char *word;
    struct list_head list;
};

static LIST_HEAD(word_list);
static DEFINE_MUTEX(word_list_mutex);
static struct timer_list word_timer;

void write_word_to_log(struct timer_list *t) {
    struct word_node *node;

    mutex_lock(&word_list_mutex);

    if (!list_empty(&word_list)) {
        node = list_first_entry(&word_list, struct word_node, list);
        printk(KERN_INFO "Word: %s\n", node->word);
        list_del(&node->list);
        kfree(node->word);
        kfree(node);
    }

    mutex_unlock(&word_list_mutex);

    mod_timer(&word_timer, jiffies + msecs_to_jiffies(1000));
}

static int __init mod_init(void) {
    printk(KERN_INFO "Module loaded\n");

    timer_setup(&word_timer, write_word_to_log, 0);
    mod_timer(&word_timer, jiffies + msecs_to_jiffies(1000));

    return 0;
}

static void __exit mod_exit(void) {
    struct word_node *node, *tmp;

    del_timer_sync(&word_timer);

    mutex_lock(&word_list_mutex);

    list_for_each_entry_safe(node, tmp, &word_list, list) {
        list_del(&node->list);
        kfree(node->word);
        kfree(node);
    }

    mutex_unlock(&word_list_mutex);

    printk(KERN_INFO "Module unloaded\n");
}

static ssize_t mod_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos) {
    char *word;
    struct word_node *node;

    word = kmalloc(count+1, GFP_KERNEL);
    if (copy_from_user(word, ubuf, count))
        return -EFAULT;
    word[count] = '\0';

    node = kmalloc(sizeof(*node), GFP_KERNEL);
    node->word = word;

    mutex_lock(&word_list_mutex);
    list_add_tail(&node->list, &word_list);
    mutex_unlock(&word_list_mutex);

    return count;
}

// We'll leave the read function as an exercise for you!

MODULE_LICENSE("GPL");

module_init(mod_init);
module_exit(mod_exit);
