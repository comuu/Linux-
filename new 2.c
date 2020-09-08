#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/type.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	pid_t pid;
	int *statlo;
	int k;
	char *msg;
	int exit_code;
	
	
	pid = fork();
	switch(pid)
	{
			case 0:
				msg = "Child process!";
				k = 5;
				exit_code = 37;
				break;
			case -1:
				msg = "Failed!";
				exit(1);
			default:
				exit_code = 0;
				break;
	}
	if(pid != 0)
	{
		int stat_val;
		pid_t child_pid;
		child_pid = wait(&stat_val);
		printf("pid = %d\n",child_pid);
		if(WIFEXITED(stat_val))
			printf("child's exit_code is %d\n",WEXITSTATUS(stat_val));
		else
			printf("nothing!");
		
	}
	else
	{
		while(k--)
		{
			puts(msg);
			sleep(1);
			
		}
	}
	exit(exit_code);
}



//setsockopt(sock_fd,IPPROTO_TCP,TCP_NODELAY,(char*)value,sizeof(int));