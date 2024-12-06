#include <linux/module.h>	// needed by all modules
#include <linux/kernel.h>	// needed by kernel functions
#include <linux/init.h>		// needed by module macros
#include <linux/uaccess.h>	// needed by user space copy functions
#include <linux/fs.h>		// needed by file operations
#include <linux/miscdevice.h>	// needed by misc device driver functions
#include <linux/ioctl.h>	// needed by ioctl functions and macros
#include <linux/delay.h>	// needed by sleep functions

#define DRIVER_AUTHOR	"CAUSW Sangjin Lee"
#define DRIVER_DESC	"driver for dotmatrix"

#define DOTM_NAME		"dotmatrix"
#define DOTM_MODULE_VERSION	"dotmatrix v1.0"
#define DOTM_ADDR		0x210

#define DOTM_MAGIC		0xBC

#define DOTM_SET_RUN		_IOW(DOTM_MAGIC, 9, int)	// JUMPEROID
#define DOTM_SET_OVER		_IOW(DOTM_MAGIC, 10, int)	// GAME OVER
#define DOTM_SET_CLEAR		_IOW(DOTM_MAGIC, 11, int)	// GAME CLEAR


// gpio fpga interface provided
extern ssize_t iom_fpga_itf_read(unsigned int addr);
extern ssize_t iom_fpga_itf_write(unsigned int addr, unsigned short int value);

// global
static int dotm_in_use = 0;

// dotmatrix fonts
unsigned char dotm_fontmap_decimal[10][10] = {
        {0x3e,0x7f,0x63,0x73,0x73,0x6f,0x67,0x63,0x7f,0x3e}, // 0
        {0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e}, // 1
        {0x7e,0x7f,0x03,0x03,0x3f,0x7e,0x60,0x60,0x7f,0x7f}, // 2
        {0xfe,0x7f,0x03,0x03,0x7f,0x7f,0x03,0x03,0x7f,0x7e}, // 3
        {0x66,0x66,0x66,0x66,0x66,0x66,0x7f,0x7f,0x06,0x06}, // 4
        {0x7f,0x7f,0x60,0x60,0x7e,0x7f,0x03,0x03,0x7f,0x7e}, // 5
        {0x60,0x60,0x60,0x60,0x7e,0x7f,0x63,0x63,0x7f,0x3e}, // 6
        {0x7f,0x7f,0x63,0x63,0x03,0x03,0x03,0x03,0x03,0x03}, // 7
        {0x3e,0x7f,0x63,0x63,0x7f,0x7f,0x63,0x63,0x7f,0x3e}, // 8
        {0x3e,0x7f,0x63,0x63,0x7f,0x3f,0x03,0x03,0x03,0x03} // 9
};

unsigned char dotm_fontmap_lee[10] = {	// lee in korean character
        0x01,0x01,0x31,0x49,0x49,0x49,0x31,0x01,0x01,0x01
};

unsigned char dotm_fontmap_sang[10] = {	// sang in korean character
        0x02,0x12,0x12,0x2b,0x4a,0x02,0x1a,0x24,0x24,0x18
};

unsigned char dotm_fontmap_jin[10] = {	// jin in korean character
        0x01,0x79,0x11,0x29,0x49,0x09,0x21,0x20,0x3f,0x00
};

unsigned char dotm_fontmap_run[5][10] = {	// JUMPEROID
        {0x38,0x08,0x08,0x48,0x30,0x00,0x12,0x12,0x12,0x0c},	// JU
        {0x28,0x7c,0x54,0x54,0x00,0x1e,0x12,0x1e,0x10,0x10},	// MP
        {0x78,0x40,0x78,0x40,0x78,0x00,0x1e,0x12,0x1c,0x12},	// ER
        {0x30,0x48,0x48,0x48,0x30,0x0e,0x04,0x04,0x04,0x0e},	// OI
        {0x70,0x48,0x48,0x48,0x70,0x00,0x00,0x00,0x00,0x00}	// D
};


unsigned char dotm_fontmap_over[4][10] = {	// GAME OVER
        {0x38,0x40,0x4c,0x44,0x38,0x00,0x0e,0x11,0x1f,0x11},	// GA
        {0x28,0x7c,0x54,0x54,0x00,0x0f,0x08,0x0f,0x08,0x0f},	// ME
        {0x30,0x48,0x48,0x48,0x30,0x00,0x11,0x11,0x0a,0x04},	// OV
        {0x78,0x40,0x78,0x40,0x78,0x00,0x1c,0x12,0x1c,0x12},	// ER
};

unsigned char dotm_fontmap_clear[5][10] = {	// GAME CLEAR
        {0x38,0x40,0x4c,0x44,0x38,0x00,0x0e,0x11,0x1f,0x11},	// GA
        {0x28,0x7c,0x54,0x54,0x00,0x0f,0x08,0x0f,0x08,0x0f},	// ME
        {0x38,0x40,0x40,0x40,0x38,0x00,0x08,0x08,0x08,0x0f},	// CL
        {0x78,0x40,0x78,0x40,0x78,0x00,0x0e,0x11,0x1f,0x11},	// EA
        {0x70,0x48,0x70,0x48,0x48,0x00,0x00,0x00,0x00,0x00}	// R
};

unsigned char *my_id[10] = {
        dotm_fontmap_decimal[2],
        dotm_fontmap_decimal[0],
        dotm_fontmap_decimal[2],
        dotm_fontmap_decimal[3],
        dotm_fontmap_decimal[1],
        dotm_fontmap_decimal[2],
        dotm_fontmap_decimal[0],
        dotm_fontmap_decimal[1],
        dotm_fontmap_decimal[6],
        dotm_fontmap_decimal[8]
};

unsigned char dotm_fontmap_full[10] = {
        0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f
};

unsigned char dotm_fontmap_empty[10] = {
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};


int dotm_open(struct inode *pinode, struct file *pfile)
{
    if (dotm_in_use !=0 ) {
        return -EBUSY;
    }

    dotm_in_use = 1;

    return 0;
}

int dotm_release(struct inode *pinode, struct file *pfile)
{
    dotm_in_use = 0;

    return 0;
}

void __dotm_write(unsigned char *arr)
{
    unsigned short wordvalue;
    int i;

    for (i=0; i < 10; i++) {
        wordvalue = arr[i] & 0x7F;
        iom_fpga_itf_write((unsigned int) DOTM_ADDR+(i*2), wordvalue);
    }
}

ssize_t dotm_write(struct inode *pinode, const char *gdata, size_t len, loff_t *off_what)
{
    int ret;
    unsigned char num;
    const char *tmp = NULL;

    tmp = gdata;

    if (len > 1) {
        printk(KERN_WARNING "only 1 byte data will be processed\n");
        len = 1;
    }

    ret = copy_from_user(&num, tmp, 1);
    if (ret < 0) {
        return -EFAULT;
    }

    __dotm_write(dotm_fontmap_decimal[num]);

    return len;
}

void print_up(unsigned char dotm_fontmaps[][10], int rows){
    unsigned char print_buf[10];
    int i, j;
    int pos = -10;

    for (i=0; i<60; i++) {
        for (j=0; j<10; j++) {
            int cur_pos = pos + j;
            if (cur_pos >= 0 && cur_pos < rows*10) {
                int row = cur_pos / 10;
                int col = cur_pos % 10;
                print_buf[j] = dotm_fontmaps[row][col];
            } else {
                print_buf[j] = 0x00;
            }
        }
        __dotm_write(print_buf);
        msleep(300);
        pos++;
    }
}

static long dotm_ioctl(struct file *pinode, unsigned int cmd, unsigned long data)
{
    msleep(500);
    switch (cmd) {
        case DOTM_SET_RUN:
            print_up(dotm_fontmap_run, 5);
            break;
        case DOTM_SET_OVER:
            print_up(dotm_fontmap_over, 4);
            break;
        case DOTM_SET_CLEAR:
            print_up(dotm_fontmap_clear, 5);
            break;
    }

    return 0;
}

static struct file_operations dotm_fops = {
        .owner		= THIS_MODULE,
        .open		= dotm_open,
        .write		= dotm_write,
        .unlocked_ioctl	= dotm_ioctl,
        .release	= dotm_release,
};

static struct miscdevice dotm_driver = {
        .fops	= &dotm_fops,
        .name	= DOTM_NAME,
        .minor	= MISC_DYNAMIC_MINOR,
};


int dotm_init(void)
{
    misc_register(&dotm_driver);
    pr_info("driver: %s driver init\n", DOTM_NAME);

    return 0;
}

void dotm_exit(void)
{
    misc_deregister(&dotm_driver);
    pr_info("driver: %s driver exit\n", DOTM_NAME);
}

module_init(dotm_init);
module_exit(dotm_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("Dual BSD/GPL");