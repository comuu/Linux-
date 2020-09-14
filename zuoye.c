int counter(int m)
{
	int count = 0;
	while(m)
	{
		if(m & 0x1)
		count++;
		else 
		continue;
		m = m >> 1;
	}
	return count;
}

bool what1(int m)
{
	int count = 0;
	while(m)
	{
		if(m & 0x1)
		count++;
		else 
		continue;
		m = m >> 1;
	}
	if(count > 1)
	return 0;
	else
	return 1;
}
#define SECONDS_PER_YEARS (1UL*365*24*60*60)
for(;;)
	
int a;
int *a;
int **a;
int a[10];
int *a[10];
int (*a)[10];
int (*absc)(int a);
int (*absc[10])(int a);


int main()
{
	int m = 12345;
	int n = 0;
	while(m > 0)
	{
		n = n*10 + m%10;
		m = m/10;
	}
	return n;
}

char *ReverseString(char *s)
{
	if(s == NULL)
	{
		return -1;
	}
	int len = strlen(s);
	int i;
	char temp;
	for(i = 0;i < len/2;i++)
	{
		temp = s[i];
		s[i] = s[len-1];
		s[len-1] = temp;
	}
	return s;
}
int recursion(int n)
{
	if(n <= 0)
		return 0;
	else if(n == 1)
		return 1;
	else
		n = n*recursion(n-1);
	return n;
}

int strcmp1(const char *src,const char *dst)
{
	if(src == NULL || dst == NULL)
		return -1;
	while(*src && *dst && *(src++) == *(dst++));
	return *src - *dst;
}

#include <stdio.h>
#include <stdlib.h>
int main(void)
{
	FILE *fd;
	char ch;
	unsigned int count = 0;
	fd = fopen("test.txt","r");
	if(fd == NULL)
	{
		return -1;
	}
	while((ch = fgetc(fd)) != EOF)
	{
		if(ch == 'a')
			count++;
	}
	printf("a的个数为：%d\n",count);
	fclose(fd);
	return 0;
}
//字符串转化为整形数
int myatoi(const char *p)
{
	if(p == NULL)
	{
		return -1;
	}
	int num = 0;
	int minus = 1;
	if(*p == '-')
	{
		minus = -1;
		p++;
	}
	while(*p != '\0')
	{
		num = num*10 + *p - '0';
		p++;
	}
	return num;	
}
void *memcpy(void *dest,const void *src,size_t n)
{
	
}












