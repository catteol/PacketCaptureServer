#include <stdio.h>
#include <signal.h>

typedef void (*sig_t) (int);

void func(int signo){
	printf("caught signal:% d\n", signo);
}
int main(){
	signal(SIGINT, func);
	signal(SIGTSTP, func);

	while(1);
}