/*另一本书*/
实现长延时更好方法事睡眠等待而不是忙等待，
unsigned long timeout = jiffies + HZ;
schedule_timeout(timeout);

用于睡眠等待的另外2个函数是：

wait_event_timeout();
//在一个特定的条件满足或者超时发生后，希望代码继续运行

msleep();//表示睡眠指定的时间

注意：长延时技术仅仅适用于进程上下文，不能用于中断上下文，因为中断上下文不允许执行
schedule()或睡眠

为了支持在将来某时刻进行某项工作，内核提供了定时器API，通过init_timer()动态定义一个定时器
也可以通过DEFINE_TIMER()静态创建定时器，然后将处理函数的地址和参数绑定给一个timer_list,并使用add_timer()注册它即可：
#include <linux/timer.h>

struct timer_list my_timer;
init_timer(&my_timer); //also see setup_timer()
my_timer.expire = jiffies + n*HZ;
my_timer.data = func_parameter;
add_timer(&my_timer);

上述代码只会让定时器运行一次，如果想让timer_func()周期执行，需要在timer_func()加上相关代码
static void timer_func(unsigned long func_parameter)
{
	init_timer(&my_timer);
	my_timer.expire = jiffies + n*HZ;
	my_timer.data = func_parameter;
	add_timer(&my_timer);
}

可以使用mod_timer()函数修改my_timer到期时间，使用del_timer()取消定时器，
或者使用timer_pending()以查看my_timer当前是否处于等待状态
用户应用程序可以使用setitimer()和getitimer()来控制报警信号在特定超时后发生


//短延时：小于jiffy的延时，在进程和中断上下文都可能发生，不能使用睡眠等待，唯一解决途径是忙等待
可以实现短延时的内核API:
mdelay();
udelay();
ndelay();
由于处理器执行一条指令的时间储存在loops_per_jiffy变量中，短延时使用loops_per_jiffy来决定它们需要循环的数量；
如下面USB主机控制器驱动程序所示:
do{
	result = ehci_readl(ehci,ptr);
	if(result == done)return 0;
	udelay(1);
	usec--;
}while(usec > 0);

时间戳计数器(TSC),记录了自启动以来处理器消耗的时钟周期数，TSC的节拍可以被转化为秒，方法是除以CPU的时钟速率（可从内核变量cpu_khz读取）
利用rdtcs()可测量某段代码的执行时间；
unsigned long low1; high1;
unsigned long low2; high2;
unsigned long exec_time;
rdtsc(low1;high1);
prink("HELLO WORLD!\n");
rdtsc(low2,high2);
exec_time = low2 - low1;

//实时钟:RTC
在非易失存储器中记录绝对时间
内核将墙上时间记录在xtime变量中，在启动过程中会根据从RTC读取到的时间初始化xtime，
可以使用do_gettimeofday()读取墙上时间；
#include <linux/time.h>
static struct timeval curr_time;
do_gettimeofday(&curr_time);


内核中的并发：
1、自旋锁和互斥体
访问共享资源的代码区域称为临界区：自旋锁(spinlock)和互斥体(mutex)是保护内核临界区的两种基本机制

自旋锁可以确保只有一个线程进入临界区，其他想进入临界区的线程必须不断原地打转直到第一个线程释放自旋锁；
这里的线程是指执行线程
例子：
#include <linux/spinlock.h>
spinlock_t mylock = SPIN_LOCK_UNLOCKED;
spin_lock(&mylock)
/* Critical Section code */
spin_unlock(&mylock);

与自旋锁不同，互斥体在进入一个被占用的临界区之前不会原地打转，而是使当前线程进入睡眠状态，如果等待时间更长互斥体比自旋锁更合适，
因为自旋锁会消耗CPU资源。在使用互斥体的场合，多于2次进程切换时间都课被认为是长时间
（1）如果临界区需要睡眠，只能使用互斥体
（2）在中断处理函数中只能使用自旋锁

互斥体例子：
include <linux/mutex.h>
/*静态声明互斥体*/
static DEFINE_MUTEX(mymutex);
//动态创建互斥体：mutex_init()
mutex_lock(&mymutex);
/* Critical Section code */
mutex_unlock(&mymutex);

//旧的信号量接口
互斥体接口代替了旧的信号量接口(semaphore)
#include <asm/semaphore.h>

static DECLARE_MUTEX(mysem);
dowm(&mysem);
/* Critical Section code */
up(&mysem);

自旋锁的irq变体
案例：
（1）进程和中断上下文，单CPU，抢占内核

另一个处于进程上下文的执行单元可能会进入临界区，所以要在进入临界区之前禁止内核抢占、中断，
并在退出临界区时恢复内核抢占和中断
代码如下：
unsigned long flags;

Point A:
	/* save interrupt state.
	disable interrupts - this implicitly disable preemption*/
	spin_lock_irqsave(&mylock,flags);
	/* Critical Section code */
	spin_unlock_irqrestore(&mylock,flags);
	
案例：
（2）进程和中断上下文，SMP机器，抢占内核
unsigned long flags;

Point A:
	/* save interrupt state.
	disable interrupts - this implicitly disable preemption*/
	spin_lock_irqsave(&mylock,flags);
	/* Critical Section code */
	spin_unlock_irqrestore(&mylock,flags);
	spin_lock(&mylock)
	spin_unlock(&mylock);
	
出了有irq变体以外，自旋锁还有底半部（BH）变体，
spin_lock_bh();
spin_unlock_bh()；


//原子操作：用于执行轻量级、仅执行一次的操作，例如修改计数器，有条件的增加值，设置位
相关函数：
set_bit();
clear_bit();
test_and_set_bit();
atomic_inc();


//读--写锁：自旋锁的读--写锁变体，如果每个执行单元在访问临界区时要么是读、要么是写共享的数据结构，但它们不会同时进行读、写操作
//允许多个读进程进入临界区 //适合读多于写的情况 

rwlock_t myrwlock = RW_LOCK_UNLOCKED;
read_lock(&myrwlock);
/*critical region */
read_unlock(&myrwlock);

/* 但是如果一个写线程进入了临界区，那么其他的读和写都不允许进入 */

rwlock_t myrwlock = RW_LOCK_UNLOCKED;
write_lock(&myrwlock);
/*critical region */
write_unlock(&myrwlock);
应用：IPX路由代码，保护路由表的并发访问

读写锁的irq变体：
read_lock_irqsave();
read_unlock_irqrestore()
write_lock_irqsave();
write_unlock_irqrestore();


//顺序锁(seqlock) 适合写多于读，写线程不必等待一个已经进入临界区的读，因此读线程会发现进入临界区的操作失败
例子：
u64 get_jiffies_64(void)
{
	unsigned long seq;
	u64 ret;
	do
	{
		seq = read_seqbegin(&xtime_lock);
		ret = jiffies_64;
		
	}while(read_seqretry(&xtime_lock,seq));//试图访问共享资源
	return ret;
}
写者会使用write_seqlock()和write_sequnlock()保护临界区


//RCU：读-复制-更新，该机制用于提高读操作远多于写操作的性能，基本理念是读线程不需要加锁
//但是写线程变得更加复杂，他会在数据结构的一份副本上执行更新操作，并代替读者看到的指针



//调试
在编译和测试代码时使能SMP(CONFIG_SMP)和抢占(CONFIG_PREEMPT)是一种很好的理念

//proc文件系统
proc文件系统(procfs)是一种虚拟的文件系统，它创建了内核内部的视窗，浏览procfs的数据是在内核运行过程产生的。
procfs中的文件可用于配置内核参数，查看内核结构体、从驱动程序中收集统计数据
这些文件中的数据由内核中相应的入口点按需动态创建，procfs中文件大小都显示为0，




//内存分配
在基于X86，一页为4096B;每一页都有一个相对应的struct page;
#include <linux/mm_types.h>
struct page{
	unsigned long flags;
	atomic_t _count;
	void *virtual;
};

1GB为内核，其实际限制为896MB
所有逻辑地址都是内核虚拟地址，所有虚拟地址并非是逻辑地址
分区：
1、ZONE_DMA(小于16MB)该区用于直接内存访问（DMA），由于传统ISA设备有24条地址线
2、ZONE_NORMAL(16~896MB),低端内存
3、ZONE_HIGH(大于896MB)，仅仅在通过kmap()映射页为虚拟地址后才能访问（通过kunmap()取消映射）为虚拟地址而非逻辑地址

kmalloc()用于从ZONE_NORMAL区域返回连续内存的内存分配函数
void *kmalloc(int count,int flags);

flags:
1、GFP_KERNEL,被进程上下文用来分配内存，kmalloc()允许睡眠，以等待其他页被释放

2、GFP_ATOMIC,被中断上下文用来获取内存，所以不允许睡眠，分配成功概率较GFP_KERNEL低
	
	由于kmalloc()返回的内存保留了以前的内容，所以将其暴露给用户空间导致安全问题
因此可使用kzalloc()获得被填充为0的内存

分配大的内存缓冲区可用vmalloc()
void *vmalloc(unsigned long count);
注意：不能用vmalloc()返回的物理上不连续的内存执行DMA




//创建内核线程：kernel_thread()
ret = kernel_thread(mykthread,NULL,CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD);

驱动程序在完成一个任务之前，不想被阻塞，因为这会降低依赖于它的应用程序的响应速度，因此驱动程序需要执行这种工作时，将相应的函数加入一个
工作函数的链表，并延后执行它

关键数据结构：
static struct __mydrv_wq{
	struct list_head mydrv_worklist;
	spinlock_t lock;
	wait_queue_head todo;
}mydrv_wq;//针对所有工作发布者的全局变量，所以需要使用并发机制以串行化同时发生的指针引用

struct _mydrv_work{
	struct list_head mydrv_workitem;
	void (*worker_func)(void *);
	void *worker_data;
}mydrv_work;

初始化数据结构
static init __init mydrv_init(void)
{
	spin_lock_init(&mydrv_wq.lock);
	//初始化等待队列
	init_waitqueue_head(&mydrv_wq.todo);
	//初始化等待队列头
	INIT_LIST_HEAD(&mydrv_wq.mydrv_worklist);
	//启动工作者线程
	kernel_thread(mydrv_work,NULL,CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD);
	return 0;
}

提交延后执行的工作
int submit_work(void (*func)(void *data),void *data)
{
	struct _mydrv_work *mydrv_work;
	mydrv_work = kmalloc(sizeof(struct __mydrv_work),GFP_ATOMIC);
	if(!mydrv_work)return -1;
	mydrv_work->worker_func = func;
	mydrv_work->worker_data = data;
	spin_lock(&mydrv_work.lock);
	list_add_tail(&mydrv_work->mydrv_workitem,&mydrv_wq.mydrv_worklist);
	wake_up(&mydrv_wq.todo);
	spin_unlock(&mydrv_wq.lock);
	return 0;
}
下面代码用于从一个驱动程序的入口点提交一个void job(void*)
submit_work(job,NULL);//将唤醒工作者线程
该线程调用list_entry()遍历链表中的所有结点，返回包含链表结点的容器数据结构

想要在循环内部删除链表元素需要调用list_for_each_entry_safe();
struct _mydrv_work *temp;
list_for_each_entry_safe(mydrv_work,temp,&mydrv_wq.mydrv_worklist,mydrv_workitem);
//使用第二个参数temp传递的临时变量保存链表中下一个入口的地址

//散列链表：仅仅需要一个包含单一指针的链表头
struct hlist_head{
	struct hlist_node *fist;
};
struct hlist_node{
	struct hlist_node *next, **pprev
}



//工作队列：用于进行延后工作的一种方式
工作队列辅助库提供了两个接口结构：workqueue_struct和work_struct,
（1）、创建一个工作队列，可以使用creat_singlethread_workqueue()创建一个服务于workqueue_struct的内核线程

（2）、创建一个工作元素，使用INIT_WORK()可以初始化一个work_struct,填充他的工作函数地址和参数

（3）、将工作元素提交给工作队列。可以通过queue_work()将work_struct提交给一个专用的work_struct,
或者通过schedule_work()提交给默认的工作者线程

//使用工作队列进行延后工作
#include <linux/workqueue.h>

struct workqueue_struct *wq;
static int __init mydrv_init(void)
{
	wq = creat_singlethread_workqueue("mydrv");
	return 0;
}
int submit_work(void (*func)(void *data),void *data)
{
	struct work_struct *hardwork;
	hardwork = kmalloc(sizeof(struct work_struct),GFP_KERNEL);
	
}






























