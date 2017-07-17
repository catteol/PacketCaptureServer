#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(){
	int sockfd, new_sockfd, length;
	struct sockaddr_in server, client;
	char buf[BUFSIZ];
	socklen_t sin_siz;
	unsigned short port;
	int pid, cpid;
	int n;
	int status;

	//ソケット生成
	if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("reader: socket");
		exit(1);
	}
	//ポートとアドレス
	server.sin_family = PF_INET;
	port = 8000;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	sin_siz = sizeof(struct sockaddr_in);

	int yes = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes)) < 0)
		{
		perror("ERROR on setsockopt");
		exit(1);
	}

	//バインド
	if(bind(sockfd, (struct sockaddr *)&server, sin_siz) < 0){
		perror("reader: bind");
		exit(1);
	}
	//コネクト待ち
	if(listen(sockfd, SOMAXCONN) < 0){
		perror("reader: listen");
		close(sockfd);
		exit(1);
	}
	for(;;){
		//accept
		if((new_sockfd = accept(sockfd, (struct sockaddr *)&client, &sin_siz)) < 0){
			perror("reader: accept");
			exit(1);
		}

		pid = fork();

		if (pid == 0){
			printf("connect from %s : %u\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
			length = recv(new_sockfd, buf, BUFSIZ, 0);
			if (n < 0) {
				perror("read");
				return 1;
			}
			length = send(new_sockfd, buf, length, 0);
			printf("<- %s", buf);
			close(new_sockfd);
			return 0;
		}else{
			while ((cpid = waitpid(-1, &status, WNOHANG)) > 0);
			if (cpid < 0) {
				perror("waitpid");
				return 1;
			}
		}
	}
	close(sockfd);
	return 0;
}