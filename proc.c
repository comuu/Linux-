所有要使用 proc的内核模块都应当包含 <linux/proc_fs.h> 头文件



proc编程中最重要的数据结构：

struct proc_dir_entry {
unsigned int low_ino;
unsigned int namelen;
const char *name; // 入口文件名
mode_t mode; // 文件访问权限模式
nlink_t nlink;
uid_t uid; // 文件的用户ID
gid_t gid; // 文件的组ID
loff_t size;
const struct inode_operations *proc_iops; // 文件Inode操作函数
/*
* NULL ->proc_fops means "PDE is going away RSN" or
* "PDE is just created". In either case, e.g. ->read_proc won't be
* called because it's too late or too early, respectively.
*
 If you're allocating ->proc_fops dynamically, save a pointer
* somewhere.
*/
const struct file_operations *proc_fops; // 文件操作函数
struct proc_dir_entry *next, *parent, *subdir; // 此入口的兄弟、父目录、和下级子入口指针
void *data; // 文件私有数据指针
read_proc_t *read_proc; // 文件读取操作函数指针
write_proc_t *write_proc; // 文件写操作函数指针
atomic_t count; /* 引用计数 */
int pde_users; /* 进程调用模块的计数 */
spinlock_t pde_unload_lock; /* proc_fops checks and pde_users bumps */
struct completion *pde_unload_completion;
struct list_head pde_openers; /* 调用 ->open, 但是未调用 ->release的进程指针 */
};

这个数据结构在内核中代表了一个proc入口，在procfs中表现为一个文件。你可以在这个结构体中看到一些文件特有的属性成员，
如uid、gid、mode、name等。但是在利用默认的proc 的API编程中，我们需要关注的是这个入口的读写函数成员：

read_proc_t *read_proc;
write_proc_t *write_proc;
在创建好proc入口之后，如果我们使用内核的默认操作函数集proc_file_operations，就需要对上面的两个成员进行初始化，
将他们映射到我们事先定义好的读写函数上即可。但是如果我们自己实现了const struct file_operations *proc_fops,那就不用在意这两个函数。
这个结构体中的其他成员基本无须我们单独来初始化，入口创建函数会帮我们处理。

创建 /proc 入口

procfs提供了一些接口函数用于在 /proc 文件系统中创建和删除一个入口文件：

struct proc_dir_entry *create_proc_entry(const char *name, mode_t mode, 
struct proc_dir_entry *parent);

name：文件名

mode：文件权限

parent：文件在 /proc 文件系统中父目录指针。

返回值是创建完成的 proc_dir_entry 指针（或者为 NULL，说明在 create 时发生了错误）。

然后通过这个返回的指针来初始化这个文件入口的其他参数，如在对该文件执行读写操作时应该调用的函数。

 如果这个/proc入口带有私有数据，以及这个数据所需要的操作函数，可以使用以下函数：

struct proc_dir_entry *proc_create_data(const char *name, mode_t mode, struct proc_dir_entry *parent, const struct file_operations *proc_fops, void *data);

name：文件名

mode：文件权限

parent：文件在 /proc 文件系统中父目录指针。

proc_fops：文件操作函数（若不使用内核的默认操作函数集proc_file_operations时，可以通过对其的初始化使用自己定义的file_operations，/proc/config.gz的实现就是一个很好的例子）

data：私有数据指针，会被赋值给所创建的proc_dir_entry的data成员。

返回值是创建完成的 proc_dir_entry 指针（或者为 NULL，说明在 create 时发生了错误）。


我们还可以使用 proc_mkdir、proc_mkdir_mode 以及 proc_symlink 在 /proc 文件系统中创建目录和软链接。

struct proc_dir_entry *proc_symlink(const char *name, struct proc_dir_entry *parent,const char *dest);
name：软链接文件名

parent：文件在 /proc 文件系统中父目录指针。

dest：软链接目标文件名


struct proc_dir_entry *proc_mkdir(const char *name, struct proc_dir_entry *parent);
name：目录名

parent：目录在 /proc 文件系统中父目录指针。

返回值是创建完成的 proc_dir_entry 指针（或者为 NULL，说明在 create 时发生了错误）。

创建的目录初始化权限是有只读和可执行权限。


struct proc_dir_entry *proc_mkdir_mode(const char *name, mode_t mode, struct proc_dir_entry *parent);
name：目录名

mode：目录权限

parent：目录在 /proc 文件系统中父目录指针。

返回值是创建完成的 proc_dir_entry 指针（或者为 NULL，说明在 create 时发生了错误）。

注意：创建 /proc 入口，有两种选择：

（1）使用proc的默认操作函数集proc_file_operations，可以使用上面的函数：

create_proc_entry

proc_create_data（其中的proc_fops=NULL）

（2）使用自己实现的file_operations，请使用：

proc_create_data（其中的proc_fops=你实现的操作函数）

在完成了/proc入口创建的之后，还有一件很重要的事情就是向已经创建的proc_dir_entry中的 write_proc 和read_proc函数指针赋值，指向实现对/proc文件节点的读写操作的函数。当某个进程访问文件时(使用 read /write系统调用), 这个请求最终会调用这些函数。所以在映射这些回调之前，我们必须已经定义好了这两个函数。

首先我们来看看read_proc的原型：

typedef int (read_proc_t)(char *page, char **start, off_t off, int count, int *eof, void *data);

当一个进程读取 /proc 文件时, 请求通过VFS传递到procfs（ __proc_file_read），procfs（ __proc_file_read）会使用__get_free_page为其分配一页内存(PAGE_SIZE 字节)作为临时缓冲，而read_proc通过向这个内存页写入数据来让procfs通过VFS返回给用户空间。

page：是procfs（ __proc_file_read）提供的数据缓存页指针，缓冲区在内核空间中，read_proc函数中可直接写入，而无须调用 copy_to_user。 

start 、off ：含义有些复杂，由于 /proc 文件可能超过缓存大小(一页)，procfs可能需要多次调用该函数来获取整个文件的数据。该函数返回时procfs（ __proc_file_read）根据start和off来判断如何将其返回给用户空间，

（1）*start = NULL

用于proc文件小于一页的情况：

off：传入参数，相对整个proc文件的偏移量，procfs（ __proc_file_read）会从page+off开始返回给用户空间

（2）NULL < *start < page

用于proc文件大于一页的情况：

off：传入参数，相对整个proc文件的偏移量，read_proc应该将文件内容偏移off开始的数据放入page起始的页中，返回后procfs（ __proc_file_read）会从page开始返回给用户空间

count：传入参数，期望read_proc返回的字节数。其实在传入前procfs已经处理过了，使其不会大于缓存大小（一页）。

eof ：简单的标志，指向一个整数, 当所有数据全部写入之后，就需要设置 eof为非零，以表示文件读取完成。

data ：表示私有数据指针, 可以用做内部用途。 

filp 参数实际上是一个打开文件结构（我们可以忽略这个参数）

buff 参数是传递给进来的字符串数据缓存。缓冲区地址实际上是一个用户空间的缓冲区，因此我们不能直接读取它，而应该使用copy_from_user。

len 参数定义了在 buff 中有多少数据要被写入。

data 参数是一个指向私有数据的指针。



删除 /proc 入口

如果要从 /proc 中删除一个文件，可以使用 remove_proc_entry 函数：


void remove_proc_entry(const char *name, struct proc_dir_entry *parent);
name：需要删除的文件名字符串

parent：这个文件在 /proc 文件系统中的位置，即父目录指针。 
 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

以上的函数是创建proc文件入口的基本流程。

对于创建一个只读的的简单 /proc 入口项来说，可以使用 create_proc_read_entry，它使用create_proc_entry创建一个 /proc 项，并用其中的read_proc参数对proc_dir_entry中的read_proc 函数指针进行初始化。

对于这样方便使用的函数，include/linux/proc_fs.h中还有许多，比如你如果要创建一个/proc/net目录下的入口文件，你可以调用：
struct proc_dir_entry *proc_net_fops_create(struct net *net, const char *name, mode_t mode, const struct file_operations *fops);
如果你要删除一个/proc/net目录下的入口文件，你可以调用：

void proc_net_remove(struct net *net, const char *name);

要创建一个/proc/net目录下的子目录，你可以调用：

struct proc_dir_entry *proc_net_mkdir(struct net *net, const char *name, struct proc_dir_entry *parent);










