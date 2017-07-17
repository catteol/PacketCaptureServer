#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main(int ac, char *av[]) {

	char buf[512];
	int nbyte = 512;
	int status;
	pid_t pid;
	char *fp;
	char *path = "/bin/";
	char exec[512];

	while(1){
		printf(">>> ");

		fp = fgets(buf, nbyte, stdin);

		if (strcmp(fp, "exit\n") == 0) {
			fprintf(stderr, "exiting...\n");
			exit(1);
		}

		if (*fp == '\n') continue;

		for(int i=0; i<strlen(buf); i++){
			if(buf[i]=='\n'){
				buf[i]='\0';
			}
		}
		strcpy(exec, path);
		strcat(exec, buf);

		pid = fork();
		if (pid == 0){
			fprintf(stderr, "I would like to execute %s\n", buf);
			execl(exec, buf, 0);
			exit(1);
		}else{
			wait(&status);
		}
	}
}