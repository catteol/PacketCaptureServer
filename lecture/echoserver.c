#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

int main(void)
{
	//ポート
	char *port = "8000";
	int err;
	struct addrinfo hints;
	struct addrinfo* res = NULL;
	struct addrinfo* ai;
	//ソケット生成
	int sockfd;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	err = getaddrinfo(NULL, port, &hints, &res);
	if(err != 0){
		perror("Failed to getaddrinfo");
		exit(1);
	}
	ai = res;
	sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (sockfd == -1) {
		perror("Failed to make socket");
		exit(1);
	}

	//already in use回避
	int yes = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes)) < 0)
		{
		perror("ERROR on setsockopt");
		exit(1);
	}

	/*IPv4 ポート・アドレス設定
	struct sockaddr_in name;
	name.sin_family = AF_INET;
	name.sin_port = (in_port_t)htons(8000);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	*/

	//バインド
	if (bind(sockfd, ai->ai_addr, ai->ai_addrlen) < 0) {
		perror("Failed to bind");
		exit(1);
	}
	//listen
	if (listen(sockfd, 1) == -1) {
		perror("Failed to listen");
	}

	struct sockaddr_storage client_addr;
	unsigned int address_size = sizeof(client_addr);
	char buf[BUFSIZ];

	while(1) {
		int pid, cpid;
		//accept
		int new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &address_size);
		if (new_sockfd == -1) {
			perror("Accept error");
		}
    //fork
		pid = fork();
		if (pid == 0){
			char *msg = "- Echo Server -\n\n";
			write(new_sockfd, msg, strlen(msg));

			int length;
			//送信されてきたデータ受け取り
			length = recv(new_sockfd, buf, BUFSIZ, 0);
			//そのまま返す
			length = send(new_sockfd, buf, length, 0);
			printf("<- %s", buf);
			exit(0);
		}else{
      //親でソケットを閉じる
			close(new_sockfd);
		}
	}
	return 0;
}