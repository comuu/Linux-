int open(const char *pathname, int flags);
open("C:\a.txt",O_CREATE,S_IPWXU);
int number = lseek(fd,0,SEEKEND);返回值为文件字节数
FILE *fopen(const char *path,const char *mode)

//fopen 与open不一样，fopen参数r,w，而open参数:O_RDONLY,O_WRONLY,O_RDWR
struct attribute{
	const char *name;
}
int val;//内核空间整型变量
get_user(val,(int *)arg);//从用户到内核