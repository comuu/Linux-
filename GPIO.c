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




















