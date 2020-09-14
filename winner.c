/*winner*/
#include <stdio.h>
#include <stdlib.h>

int main()
{	
	int i;
	int dat;
	int beforedat = -1;
	int k;
	int money[6] = {10,10,10,10,10,10};
	for(;;)
	{	
		for(i = 0;i < 6;i++)
		{
			if(money[i] <=0)
			{
				printf("编号：%d,金额不足!\n",i+1);
				printf("游戏结束了");
				return -1;
			}
		}
		
		dat = (rand()%6 + 1);
		if(dat == beforedat)
		{
			k*=2;
			printf("%d号连胜!",dat);
		}
		else
			k = 1;
		printf("赌局金额为：%d\n",k);
		printf("本局胜者为：%d\n",dat);
		for(i = 0;i < 6;i++)
		{
			if(i == dat-1)
			{
				money[dat-1] = k*5 + money[dat - 1];
				printf("编号：%d，金额：%d\n",dat,money[dat - 1]);
			}
			else
			{
				money[i] = -k;
				printf("编号：%d，金额：%d\n",i+1,money[i]);
			}
		}
		
		datbefore = dat;
	
	}
	
}