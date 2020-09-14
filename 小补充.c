/*设备驱动*/
/*设备号：32位的无符号整数（dev_t）
设备号由两部分组成：
  1 – 高12位称为主设备号，表明这个设备属于哪一类设备。
 设备号的申请方法：
第一种方法：静态定义并注册设备号
首先查看系统中有哪些设备号没有被分配给具体的设备，然后确定一个给当前的设备使用(cat /proc/devices可以看哪些号被占用了)，定义方法如下：
dev_t devno = 主设备号<<20 | 次设备号;– 低20位成为次设备号，表明这个设备是同类设备中得具体哪一个

*/
int maj = xx;
int min = xx;
dev_t devno = MKDEV(maj,min);
//注册设备号 – 使申请的设备号生效并保证设备号在Linux内核中的唯一性使用下面的接口
int register_chrdev_region(dev_t from,unsigned count,const char *name);
/*from： 要注册的设备号
count：要注册多少个设备号，例如count = 3,主设备号是255,次设备号是0，那么将按照顺序依次注册三个设备号，分别是（主：255，从：0）、（255，1）、（255，2）
name：给要注册的设备命名，注册成功可以通过cat /proc/devices查看到*/

/*第二种方法：动态申请并注册设备号*/
int alloc_chrdev_region(dev_t *dev, unsigned baseminor, unsigned count,  const char *name);
/*功能：申请一个或多个设备号并进行注册
参数：
dev：要注册的设备的设备号，输入参数，内核会寻找没有使用的设备号，然后填充到这个dev参数中。
baseminor：主设备号由内核来确定，次设备号由我们自己去确定，baseminor就对应要申请的设备号的次设备号。
count：可以一次申请多个设备号，count表示要申请的设备号的数量，当申请多个设备号时，他们的主设备号一致，次设备号会在baseminor的基础上依次加1。
name：要注册的设备的名字，注册成功可以通过cat /proc/devices查看到*/

/*在卸载模块的时候都需要将注册的设备号资源进行释放：*/
void unregister_chrdev_region(dev_t from,unsign count);

/*字符设备操作集合 – file_operations结构体
设备驱动有各种各样的， 鼠标驱动需要获取用户的坐标以及单双击动作、
LCD驱动需要写framebuffer等等，但是对上层开发调用这些驱动的人来说，
他们可能不懂也不关心底层设备是如何工作的，为了简化上层应用的操作，
驱动程序给上层提供了统一的操作接口–open、read、write等，
这样，对应做应用开发的人来说，不管你是什么样的设备，我只需要去打开（open）你这个设备，
然后进行读写等操作就可以操作这个设备了。
那么，驱动程序如何实现这样统一的接口呢？需要实现下面的file_operations结构体：
*/

struct file_operations {
       struct module *owner;
       ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
       ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
       int (*open) (struct inode *, struct file *);
       int (*release) (struct inode *, struct file *);
       ...
} 
/* owner：一般填充THIS_MODULE，表示这个驱动（模块）归自己所有（这个概念对于初学者可能难以理解，到后面我会继续说明）
open：打开设备驱动的函数指针
release：关闭设备驱动的函数指针，为了对每个设备驱动的访问保护，所以用户必须先打开设备，才能对设备进行读写等操作，操作完必须再关掉。
read：读设备驱动的函数指针（比如用户可以通过read接口读取按键驱动的按键状态等）
write：写设备驱动的函数指针（比如用户可以通过write接口写LCD驱动的framebuffer显存，将画面显示再lcd上）
用法：
定义一个属于自己设备的操作集合xxx_fops，xxx通常命名为设备的名字，例如lcd_fops, key_fops等。
 */
struct file_operations  xxx_fops ={  
       .owner   = THIS_MODULE,   //表示这个模块为自己所有
       .open    = xxx_open,      //当用户调用open接口时，内核就会根据系统调用来调用对应的xxx_fops里面的xxx_open函数（xxx表示自己命名）
       .release = xxx_close,       
       .read    = xxx_read,
       ...
};
/* 
字符设备的核心 – cdev结构体

分配、设置、注册cdev结构体
内核用cdev结构体来表示一个字符设备，所以每个字符设备驱动，都需要注册一个cdev结构体
 */
struct cdev {	
	struct kobject kobj;
  	struct module *owner;
  	const struct file_operations *ops;
  	struct list_head list;
  	dev_t dev;
  	unsigned int count;
};

/* owner：一般填充THIS_MODULE，表示这个驱动（模块）归自己所有。
ops：对应这个设备的文件操作集合。
list：内核中有很多字符设备，每个设备对应一个自己的cdev，这些cdev通过这个list连在一起，
当注册一个新的cdev时，就会通过cdev里面的list挂到内核的cdev链表上。
count：同类设备，可以一次注册多个cdev，但是他们的操作方法(fops)是一样的，比如usb设备，
多个usb共用一套操作方法(fops)，但是每个usb都有自己的cdev。 */

//分配（创建）cdev
struct cdev cdev；
//设置（初始化）cdev，函数原型
void cdev_init(struct cdev *, const struct file_operations *);
cdev_init(&cdev, &xxx_fops);
//注册cdev结构体 – 添加一个字符设备(cdev)到系统中，函数原型：
int cdev_add(struct cdev *p, dev_t dev, unsigned count)
//count: 与此设备对应的连续的次设备号的数量 – 也就是要注册的cdev的数量，当count > 1时，会向系统注册多个cdev结构体，
//这些个cdev的fops是同一个，但是设备号的次设备号不同。

//释放设备
void cdev_del(struct cdev *p)
cdev_del(&cdev);

/* 一切皆文件，用户在文件系统下看到和操作的都是文件，
但是这个文件对应在内核中是以一个inode结构体的形式存在的，
当我们在文件系统下用touch或者mknod等命令创建文件时，
内核都会创建唯一一个与之对应的inode结构体，保存这个文件的基本信息，
当我们用户操作这个文件的时候，操作系统(内核)其实操作的是对应的inode结构体，
会将我们的访问需求转换为对某个方法的调用，
根据你打开的文件的类型进行不同的操作。 */

/* 
操作系统将用户对某个文件的访问的需求转换为对某个方法的调用，内核根据你打开的文件的类型进行不同的操作，
当用户打开某个文件时，实际上内核操作的是这个文件对应的inode结构体，同时内核会创建一个file结构体与之对应，
这个file结构体里面保存了用户对这个文件（inode）结构体的操作信息（操作哪个文件：inode；以什么方式打开的，R/W/RW等：f_flags）。
总结：
也就是说，inode结构体和文件是一一对应的关系，每个文件在内核系统中都有一个唯一的inode结构体与之对应。
只有在用户对文件进行打开操作的时候，内核空间才会创建一个file结构体，
那么当多个用户对同一个文件进行打开时，就会创建多个file结构体，
分别保存每个用户的操作，file结构体和文件是多对一的关系。 */








