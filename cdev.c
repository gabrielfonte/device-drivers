#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define DEV_MEM_SIZE 512

/* Pseudo device memory */
char device_buffer[DEV_MEM_SIZE];

/* Device number */
dev_t device_number;

/* cdev structure */
struct cdev pcd_cdev;

/*  Class and device structures */
struct class *class_pcd;
struct device *device_pcd;

loff_t pcd_lseek(struct file *filep, loff_t off, int whence){
    return 0;
}

ssize_t pcd_read(struct file *filep, char __user *buff, size_t count, loff_t *f_pos){
    return 0;
}

ssize_t pcd_write(struct file *filep, const char __user *buff, size_t count, loff_t *f_pos){
    return 0;
}

int pcd_open(struct inode *inode, struct file *filep){
    return 0;
}

int pcd_release(struct inode *inode, struct file *filep){
    return 0;
}

/* File operations */
struct file_operations pcd_fops = {
    .llseek = pcd_lseek,
    .read = pcd_read,
    .write = pcd_write,
    .open = pcd_open,
    .release = pcd_release,
    .owner = THIS_MODULE
};

static int __init pcd_driver_init(void){
    /* Dynamically allocate a device number */
    alloc_chrdev_region(&device_number, 0, 1, "pcd");

    pr_info("Device number <major>:<minor> = %d:%d\n", MAJOR(device_number), MINOR(device_number));

    /* Initialize cdev structure */
    cdev_init(&pcd_cdev, &pcd_fops);
    pcd_cdev.owner = THIS_MODULE;

    /* Register cdev structure with VFS */
    cdev_add(&pcd_cdev, device_number, 1);

    /* Create a class under /sys/class */
    class_pcd = class_create("pcd_class");

    /* Create a device file under /dev */
    device_pcd = device_create(class_pcd, NULL, device_number, NULL, "pcd");

    pr_info("Module init was successful\n");

    return 0;
}

static void __exit pcd_driver_cleanup(void) {
    /* Destroy device file */
    device_destroy(class_pcd, device_number);
    /* Destroy class */
    class_destroy(class_pcd);
    /* Delete cdev structure */
    cdev_del(&pcd_cdev);
    /* Unregister character device region */
    unregister_chrdev_region(device_number, 1);

    pr_info("Module unloaded\n");
}

module_init(pcd_driver_init);
module_exit(pcd_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gabriel Fonte");
MODULE_DESCRIPTION("A pseudo character device driver");

