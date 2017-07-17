#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int pipe_fd[2];

void parent(){
	int status;
	char msg[] = "C言語辛い";

	printf("親「Oyadesu.」\n");

	close(pipe_fd[0]);
	char* p = msg;
	while (*p){
		if (write(pipe_fd[1],p,1)<0){
			perror("write");
			exit(1);
		}
		p++;
	}
	close(pipe_fd[1]);
	if(wait(&status)<0){
		perror("wait");
		exit(1);
	}
}

void child(){
	int i;
	int c;
	int len;

	printf("子「Kodomodesu」\n");

	close(pipe_fd[1]);
	while((i=read(pipe_fd[0],&c,1))>0){
		putchar(c);
	}
	putchar('\n');
	close(pipe_fd[0]);
}

int main(){
	int pid;

	if (pipe(pipe_fd)<0){
		perror("pipe");
		exit(1);
	}
	if ((pid = fork())<0){
		perror("fork");
		exit(1);
	}
	if (pid) parent();
	else child();
	return 0;
}