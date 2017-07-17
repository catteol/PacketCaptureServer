#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(void)
{
	//ソケット生成
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
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

	//ポート・アドレス設定
	struct sockaddr_in name;
	name.sin_family = AF_INET;
	name.sin_port = (in_port_t)htons(8000);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	//バインド
	if (bind(sockfd, (struct sockaddr *) &name, sizeof(name)) == -1) {
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