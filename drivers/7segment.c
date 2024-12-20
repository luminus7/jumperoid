#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/uaccess.h>          // need by user space copy function
#include <linux/fs.h>               // file operation
#include <linux/miscdevice.h>       // misc device driver function


#define DRIVER_AUTHOR   "Jin Kim"
#define DRIVER_DESC     "driver for 7-Segment"

#define SSEG_NAME            "7segment"
#define SSEG_MODULE_VERSION  "7segment V1.0"
#define SSEG_ADDR            0x004

// gpio fpga interface provided
extern ssize_t iom_fpga_itf_read(unsigned int addr);
extern ssize_t iom_fpga_itf_write(unsigned int addr, unsigned short int value);

// global
static int sseg_in_use = 0;


int sseg_open(struct inode *pinode, struct file *pfile) {
    if (sseg_in_use != 0) {
        return -EBUSY;
    }

    sseg_in_use = 1;

    return 0;
}

int sseg_release(struct inode *pinode, struct file *pfile) {
    sseg_in_use = 0;

    return 0;
}

ssize_t sseg_write(struct file *pinode, const char *gdata, size_t len, loff_t *off_what) {
    unsigned char bytevalue[4];
    unsigned short wordvalue;
    const char *tmp = gdata;

    if (copy_from_user(&bytevalue, tmp, 4)) {
        return -EFAULT;
    }

    // input data, bit shift
    wordvalue = (bytevalue[0] << 12) | (bytevalue[1] << 8) |
                (bytevalue[2] << 4) | (bytevalue[3] << 0);

    //
    iom_fpga_itf_write((unsigned int) SSEG_ADDR, wordvalue);

    return len;
}

ssize_t sseg_read(struct file *pinode, char *gdata, size_t len, loff_t *off_what) {
    unsigned char bytevalue[4];
    unsigned short wordvalue;
    char *tmp = NULL;

    tmp = gdata;

    wordvalue = iom_fpga_itf_read((unsigned int) SSEG_ADDR);

    bytevalue[0] = (wordvalue >> 12) & 0xF;
    bytevalue[1] = (wordvalue >> 8) & 0xF;
    bytevalue[2] = (wordvalue >> 4) & 0xF;
    bytevalue[3] = (wordvalue >> 0) & 0xF;


    if (copy_to_user(tmp, bytevalue, 4)) {
        return -EFAULT;
    }

    return len;
}

static struct file_operations sseg_fops = {
        .owner  = THIS_MODULE,
        .open   = sseg_open,
        .read   = sseg_read,
        .write  = sseg_write,
        .release= sseg_release,
};

static struct miscdevice sseg_driver = {
        .fops   = &sseg_fops,
        .name   = SSEG_NAME,
        .minor  = MISC_DYNAMIC_MINOR,
};


int sseg_init(void) {
    misc_register(&sseg_driver);
    printk(KERN_INFO "driver : %s driver init\n", SSEG_NAME);

    return 0;
}

void sseg_exit(void) {
    misc_deregister(&sseg_driver);
    printk(KERN_INFO "driver : %s driver exit\n", SSEG_NAME);
}

module_init(sseg_init);
module_exit(sseg_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("Dual BSD/GPL");
