#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

#define DEVICE_NAME "motion_device"
#define PIR_GPIO_PIN 14 // Replace with the GPIO pin number connected to PIR sensor

static struct task_struct *motion_thread;
static int motion_detected = 0;
static int major_number;
static struct mutex motion_mutex;
static char message[32] = "No motion\n";
static int message_len = 0;

static irqreturn_t pir_gpio_irq_handler(int irq, void *dev_id) {
    mutex_lock(&motion_mutex);
    motion_detected = 1;
    snprintf(message, sizeof(message), "Motion detected\n");
    message_len = strlen(message);
    mutex_unlock(&motion_mutex);

    printk(KERN_INFO "Motion detected by PIR sensor\n");
    return IRQ_HANDLED;
}

static ssize_t device_read(struct file *file, char __user *buffer, size_t len, loff_t *offset) {
    int ret;

    mutex_lock(&motion_mutex);

    if (*offset >= message_len) {
        mutex_unlock(&motion_mutex);
        return 0; // EOF
    }

    ret = simple_read_from_buffer(buffer, len, offset, message, message_len);

    // Reset the motion flag after reading
    if (ret > 0) {
        motion_detected = 0;
        snprintf(message, sizeof(message), "No motion\n");
        message_len = strlen(message);
    }

    mutex_unlock(&motion_mutex);
    return ret;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = device_read,
};

static int __init motion_module_init(void) {
    int irq_number;
    int ret;

    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ALERT "Failed to register character device\n");
        return major_number;
    }

    mutex_init(&motion_mutex);

    // Request the GPIO pin
    if (!gpio_is_valid(PIR_GPIO_PIN)) {
        printk(KERN_ALERT "Invalid GPIO pin\n");
        unregister_chrdev(major_number, DEVICE_NAME);
        return -ENODEV;
    }

    ret = gpio_request(PIR_GPIO_PIN, "PIR_GPIO");
    if (ret) {
        printk(KERN_ALERT "Failed to request GPIO pin\n");
        unregister_chrdev(major_number, DEVICE_NAME);
        return ret;
    }

    gpio_direction_input(PIR_GPIO_PIN);
    irq_number = gpio_to_irq(PIR_GPIO_PIN);
    if (irq_number < 0) {
        printk(KERN_ALERT "Failed to get IRQ number for GPIO pin\n");
        gpio_free(PIR_GPIO_PIN);
        unregister_chrdev(major_number, DEVICE_NAME);
        return irq_number;
    }

    ret = request_irq(irq_number, pir_gpio_irq_handler, IRQF_TRIGGER_RISING, "pir_gpio_irq", NULL);
    if (ret) {
        printk(KERN_ALERT "Failed to request IRQ\n");
        gpio_free(PIR_GPIO_PIN);
        unregister_chrdev(major_number, DEVICE_NAME);
        return ret;
    }

    printk(KERN_INFO "Motion detection module loaded. Device: /dev/%s\n", DEVICE_NAME);
    return 0;
}

static void __exit motion_module_exit(void) {
    free_irq(gpio_to_irq(PIR_GPIO_PIN), NULL);
    gpio_free(PIR_GPIO_PIN);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "Motion detection module unloaded\n");
}

module_init(motion_module_init);
module_exit(motion_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Paul Sebastien");
MODULE_DESCRIPTION("Kernel thread for motion detection with PIR sensor");
