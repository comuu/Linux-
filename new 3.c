int main(int argc, char *argv[])
{
	int i,n = argc;
	double x[n];
	for(i = 0;i < n;i++)
	x[i] = i;
	return 0;
}
switch(ch){
	case '0'...'9': c -= '0';
	break;
	case 'a'...'f': c -= 'a' - 10;
	break;
	case 'A'...'F': c -= 'A' -10;
	break;
}
struct file_opration ext2_file_operation = {
	.llseek            = generic_file_llseek,
	.read              = generic_file_read,
	.write             = generic_file_write,//标准C初始化结构体
}__attribute__((aligned(4)));//表示以4字节对齐

void example()
{
	printf("This is function",__func__);
}