#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//ポート
#define PORT 8000

int main(int argc, char *argv[]) {
	//ファイルディスクリプタのリスト
	fd_set master;
	//select用ファイルディスクリプタリスト
	fd_set read_fds;
	//鯖アドレス
	struct sockaddr_in servaddr;
	//クライアントアドレス
	struct sockaddr_in clntaddr;
	//最大ファイルディスクリプタ
	int fdmax;
	//listenソケットディスクリプタ
	int listener;
	//acceptされたソケットディスクリプタ
	int newfd;

	char buf[1024];
	int nbytes;

	//already in use回避
	int yes = 1;
	int addrlen;
	int i, n;
	//ゼロクリア
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	//ソケット作成
	if((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("Socket error");
		exit(1);
	}

	//already in use回避
	if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
		perror("setsockopt error");
		exit(1);
	}

	//バインドアドレス設定
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(PORT);
	memset(&(servaddr.sin_zero), '\0', 8);
	if(bind(listener, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1){
		perror("Bind error");
		exit(1);
	}

	//listen
	if(listen(listener, 10) == -1){
		 perror("listen error");
		 exit(1);
	}

	printf("waiting...\n");

	//listenerをリストにぶち込む
	FD_SET(listener, &master);
	//最大ファイルディスクリプタ設定
	fdmax = listener;
	for(;;){
		read_fds = master;
		if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
			perror("select error");
			exit(1);
	}

	//接続してきたのを読ませる
	for(i = 0; i <= fdmax; i++){
		if(FD_ISSET(i, &read_fds)){
			if(i == listener){
				//コネクションをつなぐ
				addrlen = sizeof(clntaddr);
				//accept
				if((newfd = accept(listener, (struct sockaddr *)&clntaddr, &addrlen)) == -1){
					perror("Accept error");
					}else{
						//リストにぶち込み
						FD_SET(newfd, &master);
						if(newfd > fdmax){
							fdmax = newfd;
						}
						printf("%s: New connection from %s on socket %d\n", argv[0], inet_ntoa(clntaddr.sin_addr), newfd);
					}
				}else{
					if((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0){
						//接続が切れたら
						if(nbytes == 0)
							printf("%s: socket %d hung up\n", argv[0], i);
						else
							perror("recv error");
						close(i);
						//リストから削除
						FD_CLR(i, &master);
					}else{
					//クライアントからデータが来たら
						for(n = 0; n <= fdmax; n++){
							//リストにいる全員に送信
							if(FD_ISSET(n, &master)){
								//listenerと自分は除く
								if(n != listener && n != i){
									if(send(n, buf, nbytes, 0) == -1)
										perror("send　error");
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;
}
