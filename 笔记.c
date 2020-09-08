static void __exit cleanup_function(void)
{
	
}
module_exit(cleanup_function)

相反, 如果你有主次编号, 需要将其转换为一个 dev_t, 使用: MKDEV(int major, int minor);

在建立一个字符驱动时你的驱动需要做的第一件事是获取一个或多个设备编号来使用. 为此目的的必要的函数是 
register_chrdev_region, 在 <linux/fs.h>中声明:
int register_chrdev_region(dev_t first, unsigned int count, char *name);
如果分配成功进行, register_chrdev_region 的返回值是 0.

内核会乐于动态为你分配一个主编号
int alloc_chrdev_region(dev_t *dev, unsigned int firstminor, unsigned int count, char *name);
dev是主编号
设备编号的释放使用: 
void unregister_chrdev_region(dev_t first, unsigned int count); 
调用 unregister_chrdev_region 的地方常常是你的模块的 cleanup 函数

内核在内部使用类型 struct cdev 的结构来代表字符设备，在<linux/cdev.h>定义

偶尔你会想将 cdev 结构嵌入一个你自己的设备特定的结构，则
void cdev_init(struct cdev *cdev, struct file_operations *fops);

任一方法, 有一个其他的 struct cdev 成员你需要初始化. 
象file_operations 结构, struct cdev 有一个拥有者成员, 应当设置为 THIS_MODULE. 一旦 cdev 结构建立, 最后的步骤是把它告诉内核, 调用:
int cdev_add(struct cdev *dev, dev_t num, unsigned int count);

为从系统去除一个字符设备, 调用: void cdev_del(struct cdev *dev);
显然, 你不应当在传递给 cdev_del 后存取 cdev 结构.

open 方法的原型是int (*open)(struct inode *inode, struct file *filp);
inode 参数有我们需要的信息,以它的 i_cdev 成员的形式, 里面包含我们之前建立的 cdev 结构. 
唯一的问题是通常我们不想要 cdev 结构本身, 我们需要的是包含 cdev 结构 的 scull_dev 结构.
在 <linux/kernel.h> 中定义:
container_of(pointer, container_type, container_field);
这个宏使用一个指向 container_field 类型的成员的指针, 它在一个 container_type 类 型的结构中, 
并且返回一个指针指向包含结构. 
struct scull_dev *dev; /* device information */ 
dev = container_of(inode->i_cdev, struct scull_dev, cdev); 
filp->private_data = dev; /* for other methods */
一旦它找到 scull_dev 结构, scull 在文件结构的 private_data 成员中存储一个它的指针, 为以后更易存取.

release 方法
设备方法应当进行下面的任务:
• 释放 open 分配在 filp->private_data 中的任何东西 
• 在最后的 close 关闭设备

scull 驱动引入2个核心函数来管理 Linux 内核中的内存. 这些函数, 定义在 <linux/slab.h>, 是:
void *kmalloc(size_t size, int flags); 
void kfree(void *ptr);
对于现在, 我们一直使用 GFP_KERNEL

scull 中的读写代码需要拷贝一整段数据到或者从用户地址空间. 这个能力由下列内核函 数提供, 它们拷贝一个任意的字节数组, 并且位于大部分读写实现的核心中.
unsigned long copy_to_user(void __user *to,const void *from,unsigned long count); 
unsigned long copy_from_user(void *to,const void __user *from,unsigned long count);

一旦你有一个定义好的 read_proc 函数, 你应当连接它到 /proc 层次中的一个入口项. 使用一个 creat_proc_read_entry 调用:
struct proc_dir_entry *create_proc_read_entry(const char *name,mode_t mode, 
struct proc_dir_entry *base, read_proc_t *read_proc, void *data);
这里, name 是要创建的文件名子, mod 是文件的保护掩码(缺省系统范围时可以作为0传递), 
base 指出要创建的文件的目录( 如果 base 是 NULL, 文件在 /proc 根下创建 ), read_proc 是实现文件的 read_proc 函数, 
data 被内核忽略( 但是传递给 read_proc). 这就是 scull 使用的调用, 来使它的 /proc 函数可用做 /proc/scullmem:

remove_proc_entry 是恢复 create_proc_read_entry 所做的事情的函数:

proc文件系统的思路是：在内核中构建一个虚拟文件系统/proc，内核运行时将内核中一些关键的数据结构以文件的方式呈现在/proc目录中的一些特定文件中，
这样相当于将不可见的内核中的数据结构以可视化的方式呈现给内核的开发者。
(6)proc文件系统给了开发者一种调试内核的方法：我们通过实时的观察/proc/xxx文件，来观看内核中特定数据结构的值。
在我们添加一个新功能的前后来对比，就可以知道这个新功能产生的影响对还是不对。

proc目录下的文件大小都是0，因为这些文件本身并不存在于硬盘中，他也不是一个真实文件，他只是一个接口，
当我们去读取这个文件时，其实内核并不是去硬盘上找这个文件，而是映射为内核内部一个数据结构被读取并且格式化成字符串返回给我们。
所以尽管我们看到的还是一个文件内容字符串，和普通文件一样的；
但是实际上我们知道这个内容是实时的从内核中数据结构来的，而不是硬盘中来的。

/proc只能读

set_file 接口假定你在创建一个虚拟文件, 它涉及一系列的必须返回给用户空间的项
第一步, 不可避免地, 是包含 <linux/seq_file.h>. 接着你必须创建4个 iterator 方 法, 称为 start, next, stop, 和 show.
void *start(struct seq_file *sfile, loff_t *pos);

void *next(struct seq_file *sfile, void *v, loff_t *pos);
这里, v 是从前一个对 start 或者 next 的调用返回的 iterator, pos 是文件的当前位 置.

void stop(struct seq_file *sfile, void *v);

内核调用 show 方法来真正输出有用的东西给用户空间. 这个方法的原型 是:
int show(struct seq_file *sfile, void *v);
int seq_printf(struct seq_file *sfile, const char *fmt, ...);

内核代码必须包含 <asm/semaphore.h>. 相关的类型是 struct semaphore
接着使用sema_init来设定它
void sema_init(struct semaphore *sem, int val); 
这里 val 是安排给旗标的初始值
如果互斥锁必须在运行时间初始化( 这是如果动态分配它的情况, 举例来说), 使用下列 中的一个:
void init_MUTEX(struct semaphore *sem); 
void init_MUTEX_LOCKED(struct semaphore *sem)

这个函数递减旗标的值,
void down(struct semaphore *sem); 
该函数用于获取信号量sem，它会导致睡眠，因此不能在中断上下文（包括IRQ上下文和softirq上下文）使用该函数。
该函数将把sem的值减1，如果信号量sem的值非负，
就直接返回，否则调用者将被挂起，直到别的任务释放该信号量才能继续运行。

int down_interruptible(struct semaphore *sem); 
down_interruptible能被信号打断，因此该函数有返回值来区分是正常返回还是被信号中断，如果返回0，
表示获得信号量正常返回，如果被信号打断，返回-EINTR。

int down_trylock(struct semaphore *sem);

void up(struct semaphore *sem); 
一旦 up 被调用, 调用者就不再拥有旗标
旗标在使用前必须初始化. scull 在加载时进行这个初始化, 在这个循环中:
for (i = 0; i < scull_nr_devs; i++) 
{ 
	scull_devices[i].quantum = scull_quantum; 
	scull_devices[i].qset = scull_qset; 
	init_MUTEX(&scull_devices[i].sem); 
	scull_setup_cdev(&scull_devices[i], i);
}
下一步, 我们必须浏览代码, 并且确认在没有持有旗标时没有对 scull_dev 数据结构的存取. 
因此, 例如, scull_write 以这个代码开始:
if (down_interruptible(&dev->sem))//可以被打断
return -ERESTARTSYS;
	注意对 down_interruptible 返回值的检查; 如果它返回非零, 操作被打断了. 
在这个情 况下通常要做的是返回 -ERESTARTSYS. 看到这个返回值后, 
内核的高层要么从头重启这个调用要么返回这个错误给用户. 

scull_write 必须释放旗标, 不管它是否能够成功进行它的其他任务. 如果事事都顺利, 执行落到这个函数的最后几行:
out:
up(&dev->sem); 
return retval;

使用 rwsem 的代码必须包含 <linux/rwsem.h>. 
读者写者旗标 的相关数据类型是 struct rw_semaphore; 
一个 rwsem 必须在运行时显式初始化:
void init_rwsem(struct rw_semaphore *sem);

只读的任务可 以并行进行它们的工作而不必等待其他读者退出临界区.
Linux 内核为这种情况提供一个特殊的旗标类型称为 rwsem (或者" reader/writer semaphore"). 
rwsem 在驱动中的使用相对较少, 但是有时它们有用
使用 rwsem 的代码必须包含 <linux/rwsem.h>. 读者写者旗标 的相关数据类型是 struct rw_semaphore; 一个 rwsem 必须在运行时显式初始化:
void init_rwsem(struct rw_semaphore *sem);
一个新初始化的 rwsem 对出现的下一个任务( 读者或者写者 )是可用的. 对需要只读存 取的代码的接口是:
void down_read(struct rw_semaphore *sem); 
int down_read_trylock(struct rw_semaphore *sem); 
void up_read(struct rw_semaphore *sem);
对 down_read 的调用提供了对被保护资源的只读存取, 与其他读者可能地并发地存取. 注意 down_read 可能将调用进程置为不可中断的睡眠. down_read_trylock 如果读存取 是不可用时不会等待; 如果被准予存取它返回非零, 否则是 0. 注意 down_read_trylock 的惯例不同于大部分的内核函数, 返回值 0 指示成功. 
一个使用 down_read 获取的 rwsem 必须最终使用 up_read 释放.

读者的接口类似:
void down_write(struct rw_semaphore *sem); 
int down_write_trylock(struct rw_semaphore *sem); 
void up_write(struct rw_semaphore *sem); 
void downgrade_write(struct rw_semaphore *sem);





等待 completion 是一个简单事来调用: 
void wait_for_completion(struct completion *c);

真正的 completion 事件可能通过调用下列之一来发出:
void complete(struct completion *c); 
void complete_all(struct completion *c);
宏定义:
INIT_COMPLETION(struct completion c);可用来快速进行这个初始化.
void complete_and_exit(struct completion *c, long retval); 通过调用 complete 来发出一个 completion 事件,
 并且为当前线程调用 exit.



自旋锁在没有打开抢占的单处理器系统上的操作被优化为什么不作, 除了改变 IRQ 屏蔽状态的那些. 由于抢占, 
甚至如果你从不希望你的代码在一个 SMP 系统上运行, 你仍需要实现正确的加锁.

自旋锁原语要求的包含文件是 <linux/spinlock.h>. 一个实际的锁有类型 spinlock_t.
void spin_lock_init(spinlock_t *lock); 在进入一个临界区前, 你的代码必须获得需要的 lock , 用: 
void spin_lock(spinlock_t *lock);
一旦你调用 spin_lock, 你将自旋直到锁变为可用.
但一个高优先级的线程自旋一个锁来等待一个低优先级的线程释放这个锁,就会造成死锁
void spin_unlock(spinlock_t *lock);

应用到自旋锁的核心规则是任何代码必须, 在持有自旋锁时, 是原子性的. 它不能睡眠; 
事实上, 它不能因为任何原因放弃处理器, 除了服务中断(并且有时即便此时也不行）

拷贝数据到或从用户空间是一个明显的例子: 请求的用户空间页可能需要在拷贝进行前从磁盘上换入, 
这个操作显然需要一个睡眠. 必须分配内存的任何操作都可能睡眠. 
kmalloc 能够决定放弃处理器, 并且等待更多内存可用除非它被明确告知不这样做

避免这个死锁需要在持有自旋锁时禁止中断

关于自旋锁使用的最后一个重要规则是自旋锁必须一直是尽可能短时间的持有. 你持有一 个锁越长, 另一个进程可能不得不自旋等待你释放它的时间越长,
它不得不完全自旋的机会越大. 长时间持有锁也阻止了当前处理器调度, 

//4个函数可以加锁一个自旋锁:
void spin_lock(spinlock_t *lock);
void spin_lock_irqsave(spinlock_t *lock, unsigned long flags);//禁止中断(只在本地处理器)在获得自 旋锁之前
void spin_lock_irq(spinlock_t *lock); 
void spin_lock_bh(spinlock_t *lock)//在获取锁之前禁 止软件中断, 但是硬件中断留作打开的.

个方法来释放一个自旋锁; 你用的那个必须对应你用来获取锁的函数. 
void spin_unlock(spinlock_t *lock);
void spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags); 
void spin_unlock_irq(spinlock_t *lock); 
void spin_unlock_bh(spinlock_t *lock);

还有一套非阻塞的自旋锁操作:
int spin_trylock(spinlock_t *lock); int spin_trylock_bh(spinlock_t *lock);
这些函数成功时返回非零( 获得了锁 ), 否则0. 没有"try"版本来禁止中断.

内核提供了一个自旋锁的读者/写者形式, 直接模仿我们在本章前面见到的读者/写者旗标.
这些锁允许任何数目的读者同时进入临界区, 但是写者必须是排他的存取. 读者写者锁有 一个类型 rwlock_t, 
在 <linux/spinlokc.h> 中定义. 它们可以以2种方式被声明和被初始化:
rwlock_t my_rwlock = RW_LOCK_UNLOCKED; /* Static way */ 
rwlock_t my_rwlock;
rwlock_init(&my_rwlock); /* Dynamic way */

void read_lock(rwlock_t *lock);
void read_lock_irqsave(rwlock_t *lock, unsigned long flags); 
void read_lock_irq(rwlock_t *lock); void read_lock_bh(rwlock_t *lock);
void read_unlock(rwlock_t *lock);
void read_unlock_irqrestore(rwlock_t *lock, unsigned long flags); 
void read_unlock_irq(rwlock_t *lock); 
void read_unlock_bh(rwlock_t *lock);

void write_lock(rwlock_t *lock);
void write_lock_irqsave(rwlock_t *lock, unsigned long flags); 
void write_lock_irq(rwlock_t *lock); 
void write_lock_bh(rwlock_t *lock); 
int write_trylock(rwlock_t *lock);
void write_unlock(rwlock_t *lock);
void write_unlock_irqrestore(rwlock_t *lock, unsigned long flags); 
void write_unlock_irq(rwlock_t *lock); 
void write_unlock_bh(rwlock_t *lock);

如果一个函数需要一个锁并且接着调用另一个函数也试图请求这个锁, 你的代码死锁.

常常可以对无锁的生产者/消费者任务有用的数据结构是环形缓存. 这个算法包含一个生 产者安放数据到一个数组的尾端, 
而消费者从另一端移走数据. 当到达数组末端, 生产者绕回到开始.
因此一个环形缓存需要一个数组和 2 个索引值来跟踪下一个新值放到哪里, 
以及哪个值在下一次应当从缓存中移走.

void atomic_set(atomic_t *v, int i); 
atomic_t v = ATOMIC_INIT(0);
设置原子变量 v 为整数值 i. 你也可在编译时使用宏定义 ATOMIC_INIT 初始化原子值.

int atomic_read(atomic_t *v); 返回 v 的当前值.
void atomic_add(int i, atomic_t *v);
由 v 指向的原子变量加 i. 返回值是 void, 因为有一个额外的开销来返回新值, 并且大部分时间不需要知道它.

void atomic_sub(int i, atomic_t *v); 从 *v 减去 i.
void atomic_inc(atomic_t *v); 
void atomic_dec(atomic_t *v);
递增或递减一个原子变量.

int atomic_inc_and_test(atomic_t *v); 
int atomic_dec_and_test(atomic_t *v); 
int atomic_sub_and_test(int i, atomic_t *v);
进行一个特定的操作并且测试结果; 如果, 在操作后, 原子值是0, 那么返回值是真; 否则, 它是假. 注意没有 atomic_add_and_test.

int atomic_add_negative(int i, atomic_t *v); 
加整数变量 i 到 v. 如果结果是负值返回值是真, 否则为假.

int atomic_add_return(int i, atomic_t *v); 
int atomic_sub_return(int i, atomic_t *v); 
int atomic_inc_return(atomic_t *v); 
int atomic_dec_return(atomic_t *v);
就像 atomic_add 和其类似函数, 除了它们返回原子变量的新值给调用者.

各种位操作是: 
void set_bit(nr, void *addr);
设置第 nr 位在 addr 指向的数据项中. 
void clear_bit(nr, void *addr);
清除指定位在 addr 处的无符号长型数据. 它的语义与 set_bit 的相反. 
void change_bit(nr, void *addr); 翻转这个位.

test_bit(nr, void *addr); 
这个函数是唯一一个不需要是原子的位操作; 它简单地返回这个位的当前值.
int test_and_set_bit(nr, void *addr); 
int test_and_clear_bit(nr, void *addr); 
int test_and_change_bit(nr, void *addr);

//2.6内核包含了一对新机制打算来提供快速地, 无锁地存取一个共享资源. seqlock 在这 种情况下工作, 
//要保护的资源小, 简单, 并且常常被存取, 并且很少写存取但是必须要快

seqlock 通常不能用在保护包含指针 的数据结构, 因为读者可能跟随着一个无效指针而写者在改变数据结构.
seqlock 定义在 <linux/seqlock.h>. 
有2个通常的方法来初始化一个 seqlock( 有 seqlock_t 类型 ):
seqlock_t lock1 = SEQLOCK_UNLOCKED;
seqlock_t lock2; 
seqlock_init(&lock2);
读存取通过在进入临界区入口获取一个(无符号的)整数序列来工作在退出时, 那个序列值与当前值比较; 如果不匹配, 读存取必须重试
写者必须获取一个排他锁来进入由一个 seqlock 保护的临界区. 为此, 调用: 
void write_seqlock(seqlock_t *lock);
写锁由一个自旋锁实现, 因此所有的通常的限制都适用. 调用: 
void write_sequnlock(seqlock_t *lock);来释放锁
void write_seqlock_irqsave(seqlock_t *lock, unsigned long flags); 
void write_seqlock_irq(seqlock_t *lock); 
void write_seqlock_bh(seqlock_t *lock);
void write_sequnlock_irqrestore(seqlock_t *lock, unsigned long flags); 
void write_sequnlock_irq(seqlock_t *lock); 
void write_sequnlock_bh(seqlock_t *lock);
还有一个 write_tryseqlock 在它能够获得锁时返回非零.

使用 RCU 的代码应当包含 <linux/rcupdate.h>.
在读这一边, 使用一个 RCU-保护的数据结构的代码应当用 rcu_read_lock 和 rcu_read_unlock 调用将它的引用包含起来. 结果就是, RCU 代码往往是象这样:
struct my_stuff *stuff; rcu_read_lock();
stuff = find_the_stuff(args...); 
do_something_with(stuff); 
rcu_read_unlock();
rcu_read_lock 调用是快的; 它禁止内核抢占但是没有等待任何东西. 在读"锁"被持有时 执行的代码必须是原子的. 在对 rcu_read_unlock 调用后, 没有使用对被保护的资源的 引用.

在改变资源完成后, 应当调用:
void call_rcu(struct rcu_head *head, void (*func)(void *arg), void *arg);
给定的 func 在释放资源是安全的时候调用; 传递给 call_rcu的是给同一个 arg. 常常, func 需要的唯一的东西是调用 kfree

以链表的"replace"操作为例，作为updater，在对copy的数据更新完成后，
需要通过rcu_assign_pointer()，用这个copy替换原节点在链表中的位置，
并移除对原节点的引用，而后调用synchronize_rcu()或call_rcu()进入grace period。因为synchronize_rcu()会阻塞等待，
所以只能在进程上下文中使用，而call_rcu()可在中断上下文中使用

作为reader，在调用rcu_read_lock()进入临界区后，因为所使用的节点可能被updater解除引用，
因而需要通过rcu_dereference()保留一份对这个节点的指针指向。进入grace period意味着数据已经更新，
而这些reader在退出临界区之前，只能使用旧的数据，也就是说，它们需要暂时忍受“过时”的数据，
不过这在很多情况下是没有多大影响的。

作为reclaimer，对于所有进入grace period之前就进入临界区的reader，需要等待它们都调用了rcu_read_unlock()退出临界区，
之后grace period结束，原节点所在的内存区域被释放。
当内存不再需要了就回收，讲到这里，你有没有觉得，
RCU的方法有点"Garbage Collection"(GC)的味道？它确实可以算一种user-driven的GC机制（区别于automatic的）。






//队列
wait_queue_head_t my_queue;//定义队列头
init_waitqueue_head(my_queue);//初始化队列头
DECLARE_WAIT_QUEUE_HEAD(name);//相当于前两个的快捷方式
DECLARE_WAITQUEUE(name,tsk);//该宏用于定义并初始化一个名为 name 的等待队列

void fastcall add_wait_queue(wait_queue_head_t *q, wait_queue_t *wait); 
void fastcall remove_wait_queue(wait_queue_head_t *q, wait_queue_t *wait);
/* add_wait_queue()用于将等待队列 wait 添加到等待队列头 q 指向的等待队列链表中，而
remove_wait_queue()用于将等待队列 wait 从附属的等待队列头 q 指向的等待队列链表中移除*/


wait_event(wait_queue_head_t queue, condition);
wait_event_interruptible(wait_queue_head_t queue, condition); 
wait_event_timeout(wait_queue_head_t queue, condition, timeout); 
wait_event_interruptible_timeout(wait_queue_head_t queue, condition, timeout);
/* 等待第 1 个参数 queue 作为等待队列头的等待队列被唤醒，
而且第 2 个参数 condition 必须满 足，否则继续阻塞。
wait_event()和 wait_event_interruptible()的区别在于后者可以被信号打断，
而前者不能。加上_timeout 后的宏意味着阻塞等待的超时时间，以 jiffy 为单位，
在第3 个参数的 timeout 到达时，不论 condition 是否满足，均返回*/

//唤醒队列
void wake_up(wait_queue_head_t *queue); 
void wake_up_interruptible(wait_queue_head_t *queue);
/*上述操作会唤醒以queue作为等待队列头的所有等待队列中所有属于该等待队列头的等待队列对应的进程。
wake_up()应该与wait_event()或 wait_event_timeout()成对使用，
而 wake_up_interruptible()
则应与 wait_event_interruptible()或 wait_event_interruptible_timeout()成对使用
。wake_up()可唤醒 处于 TASK_INTERRUPTIBLE 和 TASK_UNINTERRUPTIBLE 的进程，
而wake_up_interruptible() 只能唤醒处于TASK_INTERRUPTIBLE 的进程。*/

//在等待队列上睡眠
sleep_on(wait_queue_head_t *q ); 
interruptible_sleep_on(wait_queue_head_t *q );
/* sleep_on()函数的作用就是将目前进程的状态置成 TASK_UNINTERRUPTIBLE，
并定义一个 等待队列，之后把它附属到等待队列头 q，直到资源可获得，
q引导的等待队列被唤醒.interruptible_sleep_on()与 sleep_on()函数类似，
其作用是将目前进程的状态置成 TASK_
INTERRUPTIBLE，并定义一个等待队列，之后把它附属到等待队列头q
直到资源可获得，q 引导的等待队列被唤醒或者进程收到信号。
sleep_on()函数应该与 wake_up()成对使用，interruptible_sleep_on()应该与 wake_up_interruptible()
成对使用。*/

init_MUTEX();//初始化信号量
EAGAIN:非阻塞，再次执行

//使用非阻塞 I/O 的应用程序通常会使用 select()和 poll()系统调用查询是否可对设备进行无阻塞的访问

int select(int numfds, fd_set *readfds, fd_set *writefds, 
fd_set *exceptfds, struct timeval *timeout);
//其中 readfds、writefds、exceptfds 分别是被 select()监视的读、写和异常处理的文件描述符集
//合，numfds 的值是需要检查的号码最高的文件描述符加 1。
//timeout参数是一个指向struct timeval类型的指针，它可以使select()在等待timeout 时间后若没有文件描述符准备好则返回

//下列操作用来设置、清除、判断文件描述符集合
FD_ZERO(fd_set *set);//清除一个文件描述符集；

FD_SET(int fd,fd_set *set);//将一个文件描述符加入文件描述符集中

FD_CLR(int fd,fd_set *set);//将一个文件描述符从文件描述符集中清除

FD_ISSET(int fd,fd_set *set);//判断文件描述符是否被置位

unsigned int(*poll)(struct file * filp, struct poll_table* wait);//第1个参数为file结构体指针，
//第2个参数为轮询表指针。这个函数应该进行两项工作。对可能引起设备文件状态变化的等待队列调用 poll_wait()函数，
//将对应的等待队列头添 加到 poll_table。

void poll_wait(struct file *filp, wait_queue_heat_t *queue, poll_table * wait);
//poll_wait()函数所做的工作是把当前进程添加到 wait 参数指定的等待列表（poll_table）中。不引起阻塞
//poll()函数典型模板

static unsigned int xxx_poll(struct file *filp, poll_table *wait) 
{
	unsigned int mask = 0; 
	struct xxx_dev *dev = filp->private_data; /*获得设备结构体指针*/ ...
	poll_wait(filp, &dev->r_wait, wait);/* 加读等待队列头 */
	 poll_wait(filp, &dev->w_wait, wait);/* 加写等待队列头 */
	if (...) /* 可读 */
	 mask |= POLLIN | POLLRDNORM; /*标示数据可获得*/
	 if (...) /* 可写 */ mask |= POLLOUT | POLLWRNORM; /*标示数据可写入*/ ...
	return mask;//返回掩码
}


//支持轮询操作的poll()函数
static unsigned int globalfifo_poll(struct file *filp, poll_table *wait)
 {
	unsigned int mask = 0;
	struct globalfifo_dev *dev = filp->private_data; /*获得设备结构体指针*/
	down(&dev->sem);
	poll_wait(filp, &dev->r_wait, wait); 
	poll_wait(filp, &dev->w_wait, wait); /*fifo 非空*/
	if (dev->current_len != 0) /*fifo 非满*/
		mask |= POLLIN | POLLRDNORM; /*标示数据可获得*/
	if (dev->current_len != GLOBALFIFO_SIZE)
		mask |= POLLOUT | POLLWRNORM; /*标示数据可写入*/
	up(&dev->sem); 
	return mask;
 }
 //注意，要把 globalfifo_poll 赋给 globalfifo_fops 的 poll 成员：
 static const struct file_operation globalfifo_fops = {
	 .poll = globalfifo_poll,
 }
 
//异步通知：一旦设备就绪，则主动通知应用程序，这样应用程序根本就不需要查询
//设备状态，这一点非常类似于硬件上“中断”的概念
//信号的接收
void (*signal(int signum, void (*handler))(int)))(int);
/* 第一个参数指定信号的值，第二个参数指定针对前面信号值的处理函数，若为 SIG_IGN，表示
忽略该信号；若为 SIG_DFL，表示采用系统默认方式处理信号；若为用户自定义的函数
，则信号被捕获到后，该函数将被执行，如果 signal()调用成功，它返回最后一次为信号 signum绑定的处理函数 handler 值，失败则返
回 SIG_ERR。*/

//sigaction()函数可用于改变进程接收到特定信号后的行为，
int sigaction(int signum,const struct sigaction *act,struct sigaction *oldact));
/*该函数的第一个参数为信号的值，可以为除 SIGKILL 及 SIGSTOP 外的任何一个特定有效的
信号。第二个参数是指向结构体 sigaction 的一个实例的指针，
在结构体sigaction的实例中，指定了对特定信号的处理函数，若为空，
则进程会以缺省方式对信号处理；第三个参数oldact指向的对象用来保存原来对相应信号的处理函数，
可指定 oldact 为 NULL。如果把第二、第三个参数都 设为 NULL，那么该函数可用于检查信号的有效性*/

/*为了在用户空间中能处理一个设备释放的信号，它必须完成 3 项工作。
（1）通过 F_SETOWN IO 控制命令设置设备文件的拥有者为本进程，这样从设备驱动发出的 信号才能被本进程接收到。 
（2）通过 F_SETFL IO 控制命令设置设备文件支持 FASYNC，即异步通知模式。 
（3）通过 signal()函数连接信号和信号处理函数。
*/
//例子
signal(SIGIO, input_handler);
fcntl(STDIN_FILENO, F_SETOWN, getpid());
oflags = fcntl(STDIN_FILENO, F_GETFL);
fcntl(STDIN_FILENO, F_SETFL, oflags | FASYNC);


//设备驱动中异步通知编程,驱动应该实现fasync()函数，
//在设备资源可获得时，调用 kill_fasync()函数激发相应的信号。

//处理 FASYNC 标志变更的
int fasync_helper(int fd, struct file *filp, int mode, struct fasync_struct **fa);

//释放信号用的函数
void kill_fasync(struct fasync_struct **fa, int sig, int band);

//支持异步通知的设备结构体模板
struct xxx_dev{
	struct cdev cdev;//cdev 结构体
	struct fasync_struct *async_queue; //异步结构体指针
};

//支持异步通知的设备驱动 fasync()函数模板
static int xxx_fasync(int fd, struct file *filp, int mode)
{
	struct xxx_dev *dev = filp->private_data;
	return fasync_helper(fd, filp, mode, &dev->async_queue);
}

/*在设备资源可以获得时，应该调用 kill_fasync()释放 SIGIO 信号，
可读时第 3 个参数设置为POLL_IN，可写时第 3 个参数设置为 POLL_OUT。
 */
 //支持异步通知的设备驱动信号释放范例
static ssize_t xxx_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
struct xxx_dev *dev = filp->private_data;
 ...
/* 产生异步读信号 */ 
if (dev->async_queue) 
	kill_fasync(&dev->async_queue, SIGIO, POLL_IN);
... 
}


//最后，在文件关闭时，即在设备驱动的 release()函数中，
//应调用设备驱动的 fasync()函数 将文件从异步通知的列表中删除

//支持异步通知的设备驱动 release()函数模板
static int xxx_release(struct inode *inode, struct file *filp) 
{ 
	/* 将文件从异步通知列表中删除 */
	xxx_fasync(-1, filp, 0);	
	... 
	return 0; 
}


//异步 I/O（AIO）
//在 AIO 中，通过aiocb（AIO I/O Control Block）结构体进行区分

//aio_read()函数请求对一个有效的文件描述符进行异步读操作。这个文件描述符可以表示一个文件、套接字甚至管道。
int aio_read( struct aiocb *aiocbp );
//aio_read()函数在请求进行排队之后会立即返回。
//如果执行成功，返回值就为 0；如果出现错误，返回值就为!1，并设置 errno 的值。


//aio_write()函数用来请求一个异步写操作
int aio_write( struct aiocb *aiocbp );

//aio_error()函数被用来确定请求的状态
int aio_error( struct aiocb *aiocbp );
//EINPROGRESS：说明请求尚未完成。 ECANCELLED：说明请求被应用程序取消了。 
//-1：说明发生了错误，具体错误原因由 errno 记录


/*异步 I/O 和标准 I/O 方式之间的另外一个区别是不能立即访问这个函数的返回状态，因为异
步 I/O 并没有阻塞在 read()调用上。在标准的 read()调用中，返回状态是在该函数返回时提供的。
但是在异步 I/O 中，我们要使用 aio_return()函数。*/
ssize_t aio_return( struct aiocb *aiocbp );
/*只有在 aio_error()调用确定请求已经完成（可能成功，也可能发生了错误）之后，
才会调用这个函数。aio_return()的返回值就等价于同步情况中 read 或 write 系统调用的返回值（所传输的 字节数，
如果发生错误，返回值为负数）。
*/


/* 用户可以使用 aio_suspend()函数来挂起（或阻塞）调用进程
，直到异步请求完成为止，此时 会产生一个信号，或者发生其他超时操作。
调用者提供了一个 aiocb 引用列表，其中任何一个完 成都会导致 aio_suspend()返回*/
int aio_suspend( const struct aiocb *const cblist[], int n, const struct timespec *timeout );


/*aio_cancel()函数允许用户取消对某个文件描述符执行的一个或所有 I/O 请求*/
int aio_cancel( int fd, struct aiocb *aiocbp );
/*如果这个请求被成功取消了，那么这 个函数就会返回 AIO_CANCELED。如果请求完成了
，这个函数就会返回 AIO_NOTCANCELED
要取消对某个给定文件描述符的所有请求，用户需要提供这个文件的描述符，并将 aiocbp 参
数设置为 NULL。*/
aio_cancel(fd,NULL);

/*Lio_listio()函数可用于同时发起多个传输。这个函数非常重要，
它使得用户可以在一个系统调 用（一次内核上下文切换）中启动大量的 I/O 操作*/
int lio_listio( int mode, struct aiocb *list[], int nent, struct sigevent *sig );
/*mode 参数可以是 LIO_WAIT 或 LIO_NOWAIT。LIO_WAIT 会阻塞这个调用，
直到所有的 I/O 都完成为止。在操作进行排队之后，LIO_NOWAIT 就会返回。
list 是一个 aiocb 引用的列表，最大 元素的个数是由 nent 定义的。
如果 list 的元素为 NULL，lio_listio()会将其忽略。*/
/*例程*/
struct aiocb aiocb1, aiocb2;
struct aiocb *list[MAX_LIST];
... /* 准备第一个 aiocb */ 
aiocb1.aio_fildes = fd;
aiocb1.aio_buf = malloc( BUFSIZE+1 );
aiocb1.aio_nbytes = BUFSIZE;
aiocb1.aio_offset = next_offset;
aiocb1.aio_lio_opcode = LIO_READ; /*异步读操作，如果是异步写操作则使用LIO_WRITE*/
... /*准备多个 aiocb */ 
bzero( (char *)list, sizeof(list) ); 
 /*将 aiocb 填入链表*/ 
list[0] = &aiocb1; 
list[1] = &aiocb2; 
 ...
ret = lio_listio( LIO_WAIT, list, MAX_LIST, NULL );/*发起大量 I/O 操作*/


//使用信号作为AIO的通知
/*设置异步 I/O 请求*/
void setup_io(...)
{ 
	int fd;
	struct sigaction sig_act; 
	struct aiocb my_aiocb;
	...
	/* 设置信号处理函数 */
	sigemptyset(&sig_act.sa_mask); 
	sig_act.sa_flags = SA_SIGINFO;
	sig_act.sa_sigaction = aio_completion_handler; 
	
	/* 设置 AIO 请求 */
	bzero((char*) &my_aiocb, sizeof(struct aiocb));
	my_aiocb.aio_fildes = fd;
	my_aiocb.aio_buf = malloc(BUF_SIZE + 1); 
	my_aiocb.aio_nbytes = BUF_SIZE;
	my_aiocb.aio_offset = next_offset;
	
	/* 连接 AIO 请求和信号处理函数 */
	my_aiocb.aio_sigevent.sigev_notify = SIGEV_SIGNAL; 
	my_aiocb.aio_sigevent.sigev_signo = SIGIO;
	my_aiocb.aio_sigevent.sigev_value.sival_ptr = &my_aiocb;
	
	/* 将信号与信号处理函数绑定 */
	ret = sigaction(SIGIO, &sig_act, NULL); ...
	ret = aio_read(&my_aiocb); /*发出异步读请求*/
}

/*信号处理函数*/ 
void aio_completion_handler(int signo, siginfo_t *info, void *context)
{ 
	struct aiocb *req;
	/* 确定是我们需要的信号*/ 
	if (info->si_signo == SIGIO)
	{ 
		req = (struct aiocb*)info->si_value.sival_ptr;
		/*获得 aiocb*/ /* 请求的操作完成了吗? */ 
		if (aio_error(req) == 0) {
		/* 请求的操作完成，获取返回值 */
		ret = aio_return(req);
		}
	}
}


//中断”的概念
/*Linux 将中断处理 程序分解为两个半部：顶半部（top half）和底半部（bottom half）。
上半部 中断 （紧急的硬件操作顶半部完成尽可能少的比较紧急的功能，
它往 往只是简单地读取寄存器中的中断状态并清除中断
标志后就进行“登记中断”的工作。
“登记中断”意 味着将底半部处理程序挂到该设备的底半部执行队 列中去*/

//申请中断，顶半部
int request_irq(unsigned int irq, irq_handler_t handler, 
unsigned long irqflags, const char *devname, void *dev_id)
/*
	irq 是要申请的硬件中断号。 handler是向系统登记的中断处理函数（顶半部），
是一个回调函数，中断发生时，系统调用这个函数，dev_id 参数将被传递给它。
 irqflags 是中断处理的属性，可以指定中断的触发方式以及处理方式。
 在触发方式方面，可以是
IRQF_TRIGGER_RISING、IRQF_TRIGGER_FALLING、IRQF_TRIGGER_HIGH、IRQF_TRIGGER_ LOW 等
request_irq()返回 0 表示成功，返回-EINVAL 表示中断号无效或处理函数指针为 NULL，返
回-EBUSY 表示中断已经被占用且不能共享
 */
 
 //释放中断
 void free_irq(unsigned int irq,void *dev_id);
 
 //使能和屏蔽中断
 void disable_irq(int irq); 
 void disable_irq_nosync(int irq);
 void enable_irq(int irq);
/*disable_irq_nosync()与 disable_irq()的区别在于前者立即返回，而后者等待目前的中断处理完
成。由于 disable_irq()会等待指定的中断被处理完，因此如果在 n 号中断的顶半部调用 disable_irq(n)，
会引起系统的死锁，这种情况下，只能调用 disable_irq_nosync(n)。*/ 

//屏蔽本 CPU 内的所有中断：
#define local_irq_save(flags) ...
 void local_irq_disable(void);
//前者会将目前的中断状态保留在 flags 中（注意 flags 为 unsigned long 类型，
//被直接传递，而 不是通过指针），后者直接禁止中断而不保存状态

//恢复中断
#define local_irq_restore(flags) ...
 void local_irq_enable(void);

/*底半部：tasklet、工作队列和软中断*/

1、tasklet
void my_tasklet_func(unsigned long); /*定义一个处理函数*/ 
DECLARE_TASKLET(my_tasklet, my_tasklet_func, data); 
tasklet_schedule(&my_tasklet);
/*定义一个 tasklet 结构 my_tasklet，与 my_tasklet_func(data)函数相关联 */

2、工作队列
struct work_struct my_wq; /*定义一个工作队列*/ 
void my_wq_func(unsigned long); /*定义一个处理函数*/
//通过 INIT_WORK()可以初始化这个工作队列并将工作队列与处理函数绑定：
INIT_WORK(&my_wq, (void (*)(void *)) my_wq_func, NULL); /*初始化工作队列并将其与处理函数绑定*/
schedule_work(&my_wq);/*调度工作队列执行*/

3、软中断
用 softirq_action 结构体表征一个软中断，
这个结构体中包含软中断处理函 数指针和传递给该函数的参数。
使用 open_softirq()函数可以注册软中断对应的处理函数，
而 raise_softirq()函数可以触发一个软中断。
软中断和 tasklet 运行于软中断上下文，仍然属于原子上下文的一种，而工作队列则运行于进 程上下文。因此，软中断和 tasklet 处理函数中不能睡眠，而工作队列处理函数中允许睡眠。 
local_bh_disable()和local_bh_enable()是内核中用于禁止和使能软中断和tasklet底半部机制的函数


/*内核定时器*/
1．timer_list
2．初始化定时器
void init_timer(struct timer_list * timer);
3．增加定时器
void add_timer(struct timer_list * timer)；
4．删除定时器
int del_timer(struct timer_list * timer);
5、修改定时器的到期时间
int mod_timer(struct timer_list *timer, unsigned long expires);


内存
内存管理单元MMU：TLB和TTW

1、内核空间内存动态申请
void *kmalloc(size_t size, int flags);
//给 kmalloc()的第一个参数是要分配的块的大小，第二个参数为分配标志，用于控制 kmalloc()
//的行为。
//使用 kmalloc()申请的内存应使用 kfree()释放，最常用的分配标志是 GFP_KERNEL，其含义是在内核空间的进程中申请内存

_ _get_free_pages ()
_ _get_free_pages()系列函数/宏是 Linux 内核本质上最底层的用于获取空闲内存的方法，
因为底层的伙伴算法以page的2的n次幂为单位管理空闲内存，所以最底层的内存申请总是以页为单位的。
 _ _get_free_pages()系列函数/宏包括 get_zeroed_page()、_ _get_free_page()和_ _get_free_pages()。

_ _get_free_pages(unsigned int flags, unsigned int order);
该函数可分配多个页并返回分配内存的首地址，分配的页数为 2^order，分配的页也不清零。order 允许的最大值是 
//10（即1024页）或者11（即2048 页）


platform 设备驱动

struct platform_device 
{
const char * name;/*设备名*/
u32 id;
struct device dev; 
u32 num_resources; /* 设备所使用各类资源数量 */
struct resource * resource; /*资源 */

};

platform 设备驱动结构体
struct platform_driver {
int (*probe)(struct platform_device *); 
int (*remove)(struct platform_device *); 
void (*shutdown)(struct platform_device *);
int (*suspend)(struct platform_device *, pm_message_t state);
 int (*suspend_late)(struct platform_device *, pm_message_t state); 
 int (*resume_early)(struct platform_device *); 
 int (*resume)(struct platform_device *); struct pm_ext_ops *pm;
struct device_driver driver;
};

/*bus.type 实例*/
struct bus_type platform_bus_type = 
{
.name ="platform",
.dev_attrs = platform_dev_attrs,
.match = = platform_match,
.uevent =  platform_uevent,
.pm = PLATFORM_PM_OPS_PTR,
};
EXPORT_SYMBOL_GPL(platform_bus_type);
其中match()函数确定了platform_device和platform和platform_driver之间如何匹配
static int platform_match(struct device *dev, struct device_driver *drv) 
{ 
struct platform_device *pdev;
pdev = container_of(dev, struct platform_device, dev);
return (strncmp(pdev->name, drv->name, BUS_ID_SIZE) == 0);//匹配 platform_device 和 platform_driver 主要看两者的 name 字段是否相同
}

platform_add_devices()函数可以将平 台设备添加到系统中
int platform_add_devices(struct platform_device **devs, int num);

struct resource {
resource__size_t start;
resource_size_t end;
const char *name;
unsigned long flags; 
struct resource *parent, *sibling, *child;
};
//flags可以为 IORESOURCE_IO、IORESOURCE_MEM、IORESOURCE_IRQ、IORESOURCE_DMA 等
//如当 flags 为 IORESOURCE_MEM时，start、end 分别表示 该 platform_device 占据的内存的开始地址和结束地址

透过 platform_get_resource()这样的 API 来获取
struct resource *platform_get_resource(struct platform_device *, unsigned int, unsigned int);

platform_data 为一个 dm9000_plat_data 结构体

各种驱动与设备的连接通过dev结构体
struct dm9000_plat_data *pdata = pdev->dev.platform_data;

输入核心提供了底层输入设备驱动程序所需的 API
struct input_dev *input_allocate_device(void); 
void input_free_device(struct input_dev *dev);

注册/注销输入设备用的接口如下
int __must_check input_register_device(struct input_dev *);
void input_unregister_device(struct input_dev *);

/* 报告指定 type、code 的输入事件 */
void input_event(struct input_dev *dev, unsigned int type, unsigned int code, int value); 
/* 报告键值 */ 
void input_report_key(struct input_dev *dev, unsigned int code, int value); 
/* 报告相对坐标 */ 
void input_report_rel(struct input_dev *dev, unsigned int code, int value);
/* 报告绝对坐标 */ 
void input_report_abs(struct input_dev *dev, unsigned int code, int value); 
/* 报告同步事件 */
void input_sync(struct input_dev *dev);
而所有的输入事件，内核都用统一的数据结构来描述，这个数据结构是 input_event
struct input_event 
{ 

	struct timeval time; 
	__u16 type; 
	__u16 code; 
	__s32 value;
};


SPI 主机控制器驱动
struct spi_master { 
struct device dev; 
s16 bus_num; 
u16 num_chipselect;
/* 设置模式和时钟 */
void (*setup)(struct spi_device *spi);
/* 双向数据传输 */ 
int (*transfer)(struct spi_device *spi, struct spi_message *mesg);
void (*cleanup)(struct spi_device *spi);
};

分配、注册和注销 SPI 主机的 API 
struct spi_master * spi_alloc_master(struct device *host, unsigned size); 
int spi_register_master(struct spi_master *master); 
void spi_unregister_master(struct spi_master *master);

struct spi_driver{
	int (*probe)(struct spi_device *spi);
	int (*remove)(struct spi_device *spi);
	void (*shutdown)(struct spi_device *spi);
	int (*suspend)(struct spi_device *spi, pm_message_t mesg); 
	int (*resume)(struct spi_device *spi);
	struct device_driver driver;
}

struct spi_transfer {
	const void *tx_buf;
	void unsigned *rx_buf;
	dma_addr_t tx_dma;
	dma_addr_t rx_dma;
	unsigned len;
	unsigned cs_change:1; 
	u8 bits_per_word; 
	u16 delay_usecs;
	u32 speed_hz;
	struct list_head transfer_list;
};

spi_transfer 最终通过 spi_message 组织在一起

	struct spi_message { 
	struct list_head transfers;
	struct spi_device spi
	unsigned is_dma_mapped:1;//位域
	/* 完成被一个 callback 报告 */ 
	void (*complete)(void *context);
	void *context;
	unsigned actual_length;
	int  status;
	struct list_head queue;
	void *state;
};
而将 spi_transfer 添加到 spi_message 队列的方法则是：
void spi_message_add_tail(struct spi_transfer *t, struct spi_message *m);

使用同步API时，会阻塞等待这个消息被处理完
int spi_sync(struct spi_device *spi, struct spi_message *message);

使用异步 API 时，不会阻塞等待这个消息被处理完,但是可以在 spi_message 的 complete 字 段挂接一个回调函数，当消息被处理完成后，该函数会被调用。
int spi_async(struct spi_device *spi, struct spi_message *message);


需要一个spi_board_info文件
spi_register_board_info(nokia770_spi_board_info, ARRAY_SIZE(nokia770_spi_board_info));
/* 获得、释放时钟 */
 struct clk *clk_get(struct device *dev, const char *id); void clk_put(struct clk *clk);
/* 使能、禁止时钟 */ 
int clk_enable(struct clk *clk); void clk_disable(struct clk *clk);
/* 获得、试探和设置频率 */ 
unsigned long clk_get_rate(struct clk *clk);
long clk_round_rate(struct clk *clk, unsigned long rate); int clk_set_rate(struct clk *clk, unsigned long rate);
/* 设置、获得父时钟 */ 
int clk_set_parent(struct clk *clk, struct clk *parent); struct clk *clk_get_parent(struct clk *clk);

miscdevice 共享一个主设备号 MISC_MAJOR（即 10），但次设备号不同。所有的 miscdevice 设备形成一个链表，
对设备访问时内核根据次设备号查找对应的 miscdevice 设备，然后调用其 file_operations 结构体中注册的文件操作接口进行操作

struct miscdevice int minor;
{ 
	const char *name;
	const struct file_operations *fops;
	struct list_head list; 
	struct device *parent;
	struct device *this_device;
};

static const struct file_operations nvram_fops = {
	.owner = THIS_MODULE, 
	.llseek = nvram_llseek, 
	.read = nvram_read,
	.write = nvram_write, 
	.ioctl = nvram_ioctl,
	.open  = nvram_open,
	.release = nvram_release,
};
static struct miscdevice nvram_dev = { 
	NVRAM_MINOR, 
	"nvram",
	&nvram_fops
};
	对misddevice 的注册和注销分别通过如下两个API
	int misc_register(struct miscdevice * misc);
	int misc_deregister(struct miscdevice *misc);
	
struct sysdev_driver {
struct list_head entry;
int (*add)(struct sys_device *); 
int (*remove)(struct sys_device *); 
int (*shutdown)(struct sys_device *);
int (*suspend)(struct sys_device *, pm_message_t state); 
int (*resume)(struct sys_device *)
};	
	
注册和注销此类驱动的API 
int sysdev_driver_register(struct sysdev_class *, struct sysdev_driver *); 
void sysdev_driver_unregister(struct sysdev_class *, struct sysdev_driver *);

而此类驱动中通常会通过如下两个API 来创建和移除sysfs 的结点：
int sysdev_create_file(struct sys_device *, struct sysdev_attribute *); 
void sysdev_remove_file(struct sys_device *, struct sysdev_attribute *);

而sysdev_create_file()最终调用的是sysfs_create_file ()

sysfs_create_file ()的第一个参数为kobject 的
指针，第二个参数是一个attribute 结构体，每个attribute 对应着sysfs 中的一个文件，
而读写一个attribute 对应的文件通常需要show()和store()这两个函数
static ssize_t xxx_show(struct kobject * kobj, struct attribute * attr, char * buffer); 
static ssize_t xxx_store(struct kobject * kobj, struct attribute * attr, const char * buffer, size_t count);

Linux 设备驱动的固件加载
首先，申请固件的驱动程序发起如下请求： 
int request_firmware(const struct firmware **fw, const char *name, struct device *device);
第1个参数用于保存申请到的固件，第2个参数是固件名，第3个参数是申请固件的设备结构体。
在发起此调用后，内核的udevd会配合将固件通过对应的sysfs 结点写入内核（在设置好udev规则
的情况下）。之后内核将收到的 firmware 写入外设，最后通过如下API 释放请求： 
void release_firmware(const struct firmware *fw);


块设备驱动
block_device_operations 结构体，类似于字符设备驱动中的file_operation,它是对块设备操作的集合
struct block_device_operations {
	/*打开和释放*/
	int (*open) (struct block_device *, fmode_t); 
	int (*release) (struct gendisk *, fmode_t);
	/*io控制*/
	int (*locked_ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);
	int (*ioctl) (struct block_device *, fmode_t, unsigned, unsigned long); 
	int (*compat_ioctl) (struct block_device *, fmode_t, unsigned, unsigned long); int (*direct_access) (struct block_device *, sector_t,
	void **, unsigned long *);
	/*介质改变*/
	int (*media_changed) (struct gendisk *); 
	/*使介质有效*/
	int (*revalidate_disk) (struct gendisk *);
	/*获得驱动器信息*/
	int (*getgeo)(struct block_device *, struct hd_geometry *); 
	/*模块指针：它通常被初始化为 THIS_MODULE*/
	struct module *owner;
};

1．分配 gendisk
struct gendisk *alloc_disk(int minors);
2、增加 gendisk：gendisk结构体被分配之后，系统还不能使用这个磁盘，需要调用如下函数来注册这个磁盘设备。
void add_disk(struct gendisk *disk);
3、释放 gendisk
void del_gendisk(struct gendisk *gp);

request结构体
结构体的主要成员包括： 
sector_t hard_sector; 
unsigned long hard_nr_sectors; 
unsigned int hard_cur_sectors;
hard_sector 是第一个尚未传输的扇区，
hard_nr_sectors 是尚待完成的扇区数，
hard_cur_sectors 是当前 I/O 操作中待完成的扇区数

在块设备驱动的模块加载函数中通常需要完成如下工作。 
① 分配、初始化请求队列，绑定请求队列和请求函数。 
xxx_queue = blk_alloc_queue(GFP_KERNEL);
blk_queue_make_request(xxx_queue, &xxx_make_request); /* 绑定“制造请求”函数 */
/* 请求队列初始化 */
xxx_queue = blk_init_queue(xxx_request, xxx_lock);
② 分配、初始化 gendisk，给 gendisk 的 major、fops、queue 等成员赋值，
最后添加 gendisk。 
③ 注册块设备驱动

模块卸载函数
① 清除请求队列。 
② 删除 gendisk 和对 gendisk 的引用。 
③ 删除对块设备的引用，注销块设备驱动。


不使用请求队列
typedef int (make_request_fn) (request_queue_t *q, struct bio *bio);
这个 bio 结构体表示一个或多个要传送的缓冲区。“制造请求”函数或者直接进行传输，或者把请求重定向给其他设备。
但是在处理完成后
应该使用 bio_endio()函数通知处理结束，如下所示： 
void bio_endio(struct bio *bio, unsigned int bytes, int error);
不管对应的 I/O 处理成功与否，“制造请求”函数都应该返回0。如果“制造请求”函数返回
一个非零值，bio 将被再次提交

Linux 内核提供了一组函数用于操作 tty_driver 结构体及 tty 设备
（1）分配 tty 驱动。 
struct tty_driver *alloc_tty_driver(int lines);
这个函数返回tty_driver 指针，其参数为要分配的设备数量，line会被赋值给tty_driver的num成员

（2）注册 tty 驱动。 
int tty_register_driver(struct tty_driver *driver); 
参数为由 alloc_tty_driver()分配的 tty_driver 结构体指针，注册 tty 驱动成功时返回 0。

（3）注销 tty 驱动。 
int tty_unregister_driver(struct tty_driver *driver);

（4）注册 tty 设备。 
void tty_register_device(struct tty_driver *driver, unsigned index, struct device *device);
仅有 tty_driver 是不够的，驱动必须依附于设备，tty_register_device()函数用于注册关联于
tty_driver 的设备，index 为设备的索引（范围是 0～driver->num）

（5）注销 tty 设备。 
void tty_unregister_device(struct tty_driver *driver, unsigned index);

通过“write()系统调用—tty 核心—线路规程”的层层调用，最终调用 tty_driver 结构体中的 write()函数完成发送
	
	tty_driver 的 write()函数接受 3 个参数 tty_struct、发送数据指针及要发送的字节数，一般首先
会通过 tty_struct 的 driver_data 成员得到设备私有信息结构体，然后依次进行必要的硬件操作开始 发送
	
通过 tcgetattr()、tcsetattr()函数即可完成对终端设备的操作模式的设置和获取，这两个函数的
原型如下：
int tcgetattr (int fd, struct termios *termios_p); 
int tcsetattr (int fd, int optional_actions, struct termios *termios_p);	
	
	通过如下一组函数可完成输入/输出波特率的获取和设置： 
	speed_t cfgetospeed (struct termios *termios_p); 
	speed_t cfgetispeed (struct termios *termios_p);
	int cfsetospeed (struct termios *termios_p, speed_t speed); //设置输出波特率 
	int cfsetispeed (struct termios *termios_p, speed_t speed); //设置输入波特率

set_termios()函数需要根据用户对 termios 的设置（termios 设置包括字长、奇偶校验位、停止位、波特率等）完成实际的硬件设置。

tty_operations 中的 set_termios()函数原型为： 
void(*set_termios)(struct tty_struct *tty, struct termios *old);	
	
而一个 UART 驱动则演变为注册/注销 uart_driver，使
用如下接口： 
int uart_register_driver(struct uart_driver *drv); 
void uart_unregister_driver(struct uart_driver *drv);	
	
串口核心层提供如下函数来添加一个端口： 
int uart_add_one_port(struct uart_driver *drv, struct uart_port *port);	
对上述函数的调用应该发生在 uart_register_driver()之后
	
static int __devinit gpio_key_probe(struct platform_device *pdev)
{
	struct gpio_keys_platform_data *pdata = pdev->dev.platform_data; 
	struct gpio_keys_drvdata *ddata; 
	struct input_dev *input;
	int i,error;
	int wakeup = 0;
}

static struct uart_driver sc3c24xx_uart_drv = {
	.owner = THIS_MODULE,
	.dev_name = "s3c2410_serial",
	.nr = 3,
	.cons = s3c2410_serial_console,
	
};
static int __init s3c2410xx_serial_modinit(void)
{
	int ret;
	ret = uart_register_driver(&sc3c24xx_uart_drv);
	if(ret < 0)
	{
		printk(KERN_ERR"failed to register UART driver\n");
		return -1;
	}

	return 0;
	
}
static void __exit s3c24xx_serial_modexit(void)
{
	uart_unregister_driver(&s3c24xx_uart_drv);
}
module_init(s3c24xx_serial_modinit);
module_exit(s3c24xx_serial_modexit);


i2c_adapter 对应于物理上的一个适配器，而 i2c_algorithm 对应一套通信方法。一个 I2C 适配
器需要 i2c_algorithm 中提供的通信函数来控制适配器上产生特定的访问周期。
缺少 i2c_algorithm 的 i2c_adapter 什么也做不了，
因此 i2c_adapter 中包含其使用的 i2c_algorithm的指针。


i2c工作
提供 I2C 适配器的硬件驱动，探测、初始化 I2C适配器（如申请 I2C的 I/O地址和中断号）
驱动 CPU 控制的 I2C 适配器从硬件上产生各种信号以及处理 I2C 中断等。

提供 I2C 适配器的 algorithm，用具体适配器的 xxx_xfer()函数填充 i2c_algorithm 的 master_xfer 指针，
并把 i2c_algorithm指针赋值给 i2c_adapter 的 algo 指针。

实现 I2C 设备驱动中的 i2c_driver 接口，用具体设备 yyy 的 yyy_probe()、yyy_remove()、 yyy_suspend()、
yyy_resume()函数指针和 i2c_device_id 设备 ID 表赋值给 i2c_driver 的 probe、remove、suspend、resume 和 id_table 指针。

实现 I2C 设备所对应类型的具体驱动，i2c_driver 只是实现设备与总线的挂接，而挂接在 总线上的设备则是千差万别。
例如，如果是字符设备，就实现文件操作接口，即实现具 体设备 yyy 的 yyy_read()、yyy_write()和 yyy_ioctl()函数等；如果是声卡，就实现 ALSA 驱动。

I2C 核心中的主要函数如下
（1）增加/删除 i2c_adapter。
int i2c_add_adapter(struct i2c_adapter *adap); 
int i2c_del_adapter(struct i2c_adapter *adap);
（2）增加/删除 i2c_driver。 
int i2c_register_driver(struct module *owner, struct i2c_driver *driver); 
int i2c_del_driver(struct i2c_driver *driver); inline int i2c_add_driver(struct i2c_driver *driver);
（3）i2c_client 依附/脱离。 
int i2c_attach_client(struct i2c_client *client); 
int i2c_detach_client(struct i2c_client *client);
（4）I2C 传输、发送和接收。 
int i2c_transfer(struct i2c_adapter * adap, struct i2c_msg *msgs, int num); 
int i2c_master_send(struct i2c_client *client,const char *buf ,int count); 
int i2c_master_recv(struct i2c_client *client, char *buf ,int count);

I2C 总线驱动模块的加载函数要完成两个工作。 
初始化 I2C 适配器所使用的硬件资源，如申请 I/O 地址、中断号等
通过 i2c_add_adapter()添加 i2c_adapter 的数据结构，当然这个 i2c_adapter 数据结构的成 员已经被 xxx 适配器的相应函数指针所初始化。
I2C 总线驱动模块的卸载函数要完成的工作与加载函数相反。 
释放 I2C 适配器所使用的硬件资源，如释放 I/O 地址、中断号等。 
通过 i2c_del_adapter()删除 i2c_adapter 的数据结构。


在设计具体的网络设备驱动程序时，我们需要完成的主要工作是编写设备驱动功能层的相关
函数以填充 net_device 数据结构的内容并将 net_device 注册入内核。

Linux 套接字缓冲区支持分配、释放、变更
等功能函数
Linux 内核用于分配套接字缓冲区的函数有：
struct sk_buff *alloc_skb(unsigned int len, gfp_t priority) 
struct sk_buff *dev_alloc_skb(unsigned int len); 
alloc_skb()函数分配一个套接字缓冲区和一个数据缓冲区，参数 len 为数据缓冲区的空间大小，
通常以 L1_CACHE_BYTES 字节（对于ARM为32)对齐，参数 priority 为内存分配的优先级。 dev_alloc_skb()函数以 GFP_ATOMIC 优先级进行 skb 的分配

Linux 内核用于释放套接字缓冲区的函数有：
void kfree_skb(struct sk_buff *skb); 
void dev_kfree_skb(struct sk_buff *skb);
void dev_kfree_skb_irq(struct sk_buff *skb); 
void dev_kfree_skb_any(struct sk_buff *skb);

Linux 内核中可以用如下函数在缓冲区尾部增加数据： 
unsigned char *skb_put(struct sk_buff *skb, unsigned int len); 
它会导致 skb->tail 后移 len，而 skb->len 增加 len 的大小。通常，设备驱动的收数据处理中会
调用此函数。

Linux 内核中可以用如下函数在缓冲区开头增加数据： 
unsigned char *skb_push(struct sk_buff *skb, unsigned int len); 
它会导致 skb->data 前移 len，而 skb->len 增加 len 的大小

int (*init)(struct net_device *dev); 
init 为设备初始化函数指针，如果这个指针被设置了，则网络设备被注册时将调用该函数完
成对 net_device 结构体的初始化。但是，设备驱动程序可以不实现这个函数并将其赋值为 NULL

hard_start_xmit()函数会启动数据包的发送，当系统调用驱动程序的 hard_start_xmit()函数时，
需要向其传入一个 sk_buff结构体指针，以使得驱动程序能获取从上层传递下来的数据包。 
void (*tx_timeout)(struct net_device *dev); 
当数据包的发送超时时，tx_timeout ()函数会被调用，该函数需采取重新启动数据包发送过程
或重新启动硬件等措施来恢复网络设备到正常状态。
	
struct net_device_stats* (*get_stats)(struct net_device *dev); get_stats()函数用于获得网络设备的状态信息，它返回一个 net_device_stats 结构体。
net_device_stats 结构体保存了网络设备详细的流量统计信息，如发送和接收到的数据包数、字节 数等
int (*do_ioctl)(struct net_device *dev, struct ifreq *ifr, int cmd); 
int (*set_config)(struct net_device *dev, struct ifmap *map); 
int (*set_mac_address)(struct net_device *dev, void *addr);	
	
static inline void netif_napi_add(struct net_device *dev, struct napi_struct *napi, int (*poll)(struct napi_struct *, int), int weight);
static inline void netif_napi_del(struct napi_struct *napi); 
以上两个函数分别用于初始化和移除一个 NAPI，netif_napi_add()的 poll 参数是 NAPI 要调度执行的轮询函数。
static inline void napi_enable(struct napi_struct *n); 
static inline void napi_disable(struct napi_struct *n); 
以上两个函数分别用于使能和禁止 NAPI 调度。 
static inline int napi_schedule_prep(struct napi_struct *n);
	
网络设备驱动的注册与注销使用成对出现的 register_netdev()和 unregister_netdev()函数完成
nt register_netdev(struct net_device *dev); void unregister_netdev(struct net_device *dev); 这两个函数都接收一个 net_device 结构体指针为参数，
可见 net_device 数据结构在网络设备 驱动中的核心地位。

完成与 alloc_enetdev()和 alloc_etherdev()函数相反功能，即释放 net_device 结构体的函数为： void free_netdev(struct net_device *dev); net_device结构体的分配和网络设备驱动注册需在网络设备驱动程序的模块加载函数中进行，
而 net_device 结构体的释放和网络设备驱动的注销则需在模块卸载函数中完成	
	
Linux 内核提供的 netif_start_queue()和 netif_stop_queue()两个函数的原型为：
void netif_start_queue(struct net_device *dev); 
void netif_stop_queue (struct net_device *dev);
	
Linux 网络子系统在发送数据包时，会调用驱
动程序提供的 hard_start_transmit()函数，该函数用于启动数据包的发送。
在设备初始化的时候，这个函数指针需被初始化指向设备的 xxx_tx()函数。	
	
（1）网络设备驱动程序从上层协议传递过来的 sk_buff 参数获得数据包的有效数据和长度， 将有效数据放入临时缓冲区。 
（2）对于以太网，如果有效数据的长度小于以太网冲突检测所要求数据帧的最小长度ETH_ZLEN，则给临时缓冲区的末尾填充 0。 
（3）设置硬件的寄存器，驱使网络设备进行数据发送操作。	
	
当数据传输超时时，意味着当前的发送操作失败或硬件已陷入未知状态，此时，数据包发送
超时处理函数 xxx_tx_timeout()将被调用。
这个函数也需要调用 Linux 内核提供的 netif_wake_ queue()函数重新启动设备发送队列
	
网络设备接收数据的主要方法是由中断引发设备的中断处理函数，中断处理函数判断中断类
型，如果为接收中断，则读取接收到的数据，分配 sk_buffer 数据结构和数据缓冲区，将接收到的 数据复制到数据缓冲区，
并调用 netif_rx()函数将 sk_buffer 传递给上层协议	
	
	
int register_sound_mixer(struct file_operations *fops, int dev); 
上述函数用于注册一个混音器，第一个参数 fops 即是文件操作接口，第二个参数 dev 是设备 编号，如果填入-1，
则系统自动分配一个设备编号
	
dsp 接口 file_operations 中的 ioctl()函数处理对采样率、量化精度、DMA 缓冲区块大小等参数
设置 I/O 控制命令的处理。

通常会使用DMA，DMA对声卡而言非常重要。
例如，在放音时，驱动设置完DMA 控制器的源数据地址（内存中的DMA 缓冲区）、目的地址（音 频控制器 FIFO）和 DMA 的数据长度，
DMA 控制器会自动发送缓冲区的数据填充 FIFO，直到发送完相应的数据长度后才中断一次。
	
	
7．注册与注销帧缓冲设备 
Linux 内核提供了 register_framebuffer()和 unregister_framebuffer()函数分别注册和注销帧缓冲设备，
这两个函数都接受 FBI 指针为参数，原型为： 
int register_framebuffer(struct fb_info *fb_info); 
int unregister_framebuffer(struct fb_info *fb_info); 
对于 register_framebuffer()函数而言，如果注册的帧缓冲设备数超过了 FB_MAX（目前定义为
32），则函数返回-ENXIO，注册成功则返回 0。	
	
	
	
	