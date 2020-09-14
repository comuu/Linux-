/*例子*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#define NUM_OF_DEVICES  2

int major = 25;

struct hello_device{
	dev_t devno;
	struct cdev cdev;
	char data[128];
	char name[16];
}hello_dev[NUM_OF_DEVICES];

struct class *hello_class;

const char DEVNAME[] = "helo_device";

int hello_open(struct inode *ip,struct file *fp)
{
	printk("%s: %d\n",__func__,__LINE__);
	struct hello_device *dev = container_of(ip->i_cdev,struct hello_device,cdev);
	
	fp->private_data = dev;
	return 0;
}
ssize_t hello_read(struct file *fp,char __user *buf,size_t count,loff_t *loff)
{
	int ret;
	struct hello_device *dev = fp->private_data;
	
	prink("%s : %d\n",__func__,__LINE__);
	if(count <= 0 || count > 128)
		count = 128;
	if((ret = copy_to_user(buf,dev->data,count)))
	{
		printk("copy_to_user error!");
		return -1;
	}
}

ssize_t hello_write(struct file *fp,char __user *buf,size_t count,loff_t loff)
{
	int ret;
	struct hello_device *dev = fp->private_data;
	
	prink("%s : %d\n",__func__,__LINE__);
	if(count <= 0 || count > 128)
		count = 128;
	if((ret = copy_from_user(buf,dev->data,count)))
	{
		printk("copy_to_user error!");
		return -1;
	}
}
struct file_operation hello_fops = {
	.owner = THIS_MODULE,
	.open = hello_open,
	.release = hello_close,
	.read = hello_read,
	.write = hello_write
};

struct cdev cdev;

static int hello_init(void)
{
	int i;
	printk("%s : %d\n",__func__,__LINE__);
	for(i = 0; i < NUM_OF_DEVICES; i++)
	{
		hello_dev[i].devno = MKDEV(major,i);
		sprintf(hello_dev[i].name,"%s%d",DEVNAME,i);
		register_chredev_region(hello_dev[i].devno,1,hello_dev[i].name);
		hello_dev[i].cdev.owner = THIS_MODULE;
		cdev_add(&hello_dev[i].cdev,hello_dev[i].devno,1);
		cdev_init(&hello_dev[i].cdev,&hello_fops);
		sprintf(hello_dev[i].data,"hi,I %d",i);	
	}
	hello_class = class_create(THIS_MODULE,DEVNAME);
	for(i = 0; i < NUM_OF_DEVICES; i++)
	{
		device_create(hello_class,NULL,hello_dev[i].devno,NULL,"%S%d",DEVNAME,i);
		printk("success!\n");
	}
	return 0；
}
 static void hello_exit(void)
 {
	int i;
	printk("%s : %d\n",__func__,__LINE__);
	for(i = 0; i < NUM_OF_DEVICES; i++)
	{
		device_destroy(hello_class,hello_dev[i].devno);
		cdev_del(&hello_dev[i].cdev);
		unregister_chrdev_region(hello_dev[i].devno,1);
		
	}
	class_destroy(hello_class);
 }
MODULE_LICENSE("GPL");
module_init(hello_init);
module_exit(hello_exit);

__attribute__(at )


