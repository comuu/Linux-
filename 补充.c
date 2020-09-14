/*补充*/
1.关于wait_event_interruptible() 和 wake_up()的使用

读一下wait_event_interruptible()的源码，不难发现这个函数先将
当前进程的状态设置成TASK_INTERRUPTIBLE，然后调用schedule()，
而schedule()会将位于TASK_INTERRUPTIBLE状态的当前进程从runqueue
队列中删除。从runqueue队列中删除的结果是，当前这个进程将不再参
与调度，除非通过其他函数将这个进程重新放入这个runqueue队列中，
这就是wake_up()的作用了。
  
由于这一段代码位于一个由condition控制的for(;;)循环中，所以当由
shedule()返回时(当然是被wake_up之后，通过其他进程的schedule()而
再次调度本进程)，如果条件condition不满足，本进程将自动再次被设
置为TASK_INTERRUPTIBLE状态，接下来执行schedule()的结果是再次被
从runqueue队列中删除。这时候就需要再次通过wake_up重新添加到
runqueue队列中。
  
如此反复，直到condition为真的时候被wake_up.
  
可见，成功地唤醒一个被wait_event_interruptible()的进程，需要满足：
 在 
 1)condition为真的前提下，
 2) 调用wake_up()。

所以，如果你仅仅修改condition，那么只是满足其中一个条件，这个时候，
被wait_event_interruptible()起来的进程尚未位于runqueue队列中，因
此不会被 schedule。这个时候只要wake_up一下就立刻会重新进入运行调度。
  
#include <linux/jiffies.h> 
unsigned long j, stamp_1, stamp_half, stamp_n;
j = jiffies; /* read the current value */ 
stamp_1 = j + HZ; /* 1 second in the future */ 
stamp_half = j + HZ/2; /* half a second */ 
stamp_n = j + n * HZ / 1000; /* n milliseconds */


#include <linux/jiffies.h>
int time_after(unsigned long a, unsigned long b); 
int time_before(unsigned long a, unsigned long b); 
int time_after_eq(unsigned long a, unsigned long b); 
int time_before_eq(unsigned long a, unsigned long b);
第一个当 a, 作为一个 jiffies 的快照, 代表 b 之后的一个时间时, 取值为真, 第二个 当 时间 a 在时间 b 之前时取值为真, 以及最后 2 个比较"之后或相同"和"之前或相同". 这个代码工作通过转换这个值为 signed long, 减它们, 并且比较结果. 如果你需要以一 种安全的方式知道 2 个 jiffies 实例之间的差, 你可以使用同样的技巧: diff = (long)t2 - (long)t1;.
你可以转换一个 jiffies 差为毫秒, 一般地通过: msec = diff * 1000 / HZ;

它们打算使用 struct timeval 和 struct timespec 来表示时间. 这2个结构代表一个精确的时间量, 
使用2个成员: seconds 和 microseconds

#include <linux/time.h>
unsigned long timespec_to_jiffies(struct timespec *value); 
void jiffies_to_timespec(unsigned long jiffies, struct timespec *value);
unsigned long timeval_to_jiffies(struct timeval *value);

#include <linux/jiffies.h> 
u64 get_jiffies_64(void);

最有名的计数器寄存器是 TSC ( timestamp counter), 在 x86 处理器中随 Pentium 引 入的并且在所有从那之后的 CPU 中出现 -- 包括 x86_64 平台. 
它是一个64-位寄存器计数CPU的时钟周期; 它可从内核和用户空间读取.

在包含了 <asm/msr.h> (一个 x86-特定的头文件, 它的名子代表"machine-specific registers"), 你可使用一个这些宏:
rdtsc(low32,high32); 
rdtscl(low32); 
rdtscll(var64);
第一个宏自动读取64-位值到2个32-位 变量; 下一个("read low half") 读取寄存 器的低半部到一个 32-位 变量, 丢弃高半部; 最后一个读64-位 值到一个 long long 变量,
 由此得名. 所有这些宏存储数值到它们的参数中

#include <linux/timex.h> cycles_t get_cycles(void);
这个函数为每个平台定义, 并且它一直返回0在没有周期-计数器寄存器的平台上. 
cycles_t 类型是一个合适的 unsigned 类型来持有读到的值.

#include <linux/time.h> 
struct timespec current_kernel_time(void);

如果你的驱动使用一个等待队列来等待某些其他事件, 但是你也想确保它在一个确定时间 段内运行, 可以使用 wait_event_timeout 或者 wait_event_interruptible_timeout:
#include <linux/wait.h>
long wait_event_timeout(wait_queue_head_t q, condition, long timeout); 
long wait_event_interruptible_timeout(wait_queue_head_t q, condition, long timeout);
这些函数在给定队列上睡眠, 但是它们在超时(以 jiffies 表示)到后返回. 因此,它们 实现一个限定的睡眠不会一直睡下去.
 注意超时值表示要等待的 jiffies 数, 不是一个 绝对时间值. 这个值由一个有符号的数表示, 因为它有时是一个相减运算的结果, 尽管这 些函数如果提供的超时值是负值通过一个 printk 语句抱怨. 如果超时到, 这些函数返回 0; 如果这个进程被其他事件唤醒, 它返回以 jiffies 表示的剩余超时值. 
返回值从不会 是负值, 甚至如果延时由于系统负载而比期望的值大.

#include <linux/sched.h> 
signed long schedule_timeout(signed long timeout);
这里, timeout是要延时的jiffies 数. 返回值是0 除非这个函数在给定的 timeout 流失前返回(响应一个信号). schedule_timeout 
请求调用者首先设置当前的进程状态, 因此一个典型调用看来如此

例子
set_current_state(TASK_INTERRUPTIBLE); 
schedule_timeout (delay);
在刚刚展示的例子中, 第一行调用 set_current_state 来设定一些东西以便调度器不会 再次运行当前进程, 直到超时将它置回 TASK_RUNNING 状态. 为获得一个不可中断的延时, 使用 TASK_UNINTERRUPTIBLE 代替. 如果你忘记改变当前进程的状态, 调用 schedule_time 如同调用 shcedule( 即, jitsched 的行为), 
建立一个不用的定时器

#include <linux/delay.h>
void ndelay(unsigned long nsecs); 
void udelay(unsigned long usecs); 
void mdelay(unsigned long msecs);
这些函数的实际实现在 <asm/delay.h>, 是体系特定的, 并且有时建立在一个外部函数上. 
每个体系都实现 udelay, 但是其他的函数可能或者不可能定义; 如果它们没有定义

有另一个方法获得毫秒(和更长)延时而不用涉及到忙等待. 文件 <linux/delay.h> 声明 这些函数:
void msleep(unsigned int millisecs);
unsigned long msleep_interruptible(unsigned int millisecs); void ssleep(unsigned int seconds)
前 2 个函数使调用进程进入睡眠给定的毫秒数. 一个对 msleep 的调用是不可中断的; 
你能确保进程睡眠至少给定的毫秒数. 如果你的驱动位于一个等待队列并且你想唤醒来打断睡眠, 
使用 msleep_interruptible. 从 msleep_interruptible 的返回值正常地是0; 如果, 
但是, 这个进程被提早唤醒, 返回值是在初始请求睡眠周期中剩余的毫秒数. 
对 ssleep 的调用使进程进入一个不可中断的睡眠给定的秒数.

内核代码能够告知是否它在中断上下文中运行, 通过调用函数 in_interrupt(), 
它不要参数并且如果处理器当前在中断上下文运行就返回非零

你应当真正考虑是否 in_atomic 是你实际想要的.
2个函数都在<asm/hardirq.h> 中声明.

#include <linux/timer.h> 
struct timer_list {
/* ... */
	unsigned long expires;
	void (*function)(unsigned long); 
	unsigned long data;
}; 
void init_timer(struct timer_list *timer); 
struct timer_list TIMER_INITIALIZER(_function, _expires, _data);
void add_timer(struct timer_list * timer); 
int del_timer(struct timer_list * timer);







