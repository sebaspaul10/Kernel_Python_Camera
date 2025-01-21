#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define DEVICE_NAME "motion_device"

static struct task_struct *motion_thread;
static int motion_detected = 0;
static int major_number;
static struct mutex motion_mutex;

static int motion_thread_function(void *data) {
    while (!kthread_should_stop()) {
        ssleep(5); // Simulate motion detection every 5 seconds
        mutex_lock(&motion_mutex);
        motion_detected = 1; // Set motion detected flag
        mutex_unlock(&motion_mutex);
        printk(KERN_INFO "Motion detected by kernel thread\n");
    }
    return 0;
}

static ssize_t device_read(struct file *file, char __user *buffer, size_t len, loff_t *offset) {
    int ret;
    char msg[32];

    mutex_lock(&motion_mutex);
    if (motion_detected) {
        snprintf(msg, sizeof(msg), "Motion detected\n");
        motion_detected = 0; // Reset the flag
    } else {
        snprintf(msg, sizeof(msg), "No motion\n");
    }
    mutex_unlock(&motion_mutex);

    ret = simple_read_from_buffer(buffer, len, offset, msg, strlen(msg));
    return ret;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = device_read,
};

static int __init motion_module_init(void) {
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ALERT "Failed to register character device\n");
        return major_number;
    }

    mutex_init(&motion_mutex);

    motion_thread = kthread_run(motion_thread_function, NULL, "motion_thread");
    if (IS_ERR(motion_thread)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create kernel thread\n");
        return PTR_ERR(motion_thread);
    }

    printk(KERN_INFO "Motion detection module loaded\n");
    return 0;
}

static void __exit motion_module_exit(void) {
    kthread_stop(motion_thread);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "Motion detection module unloaded\n");
}

module_init(motion_module_init);
module_exit(motion_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel thread for motion detection");
