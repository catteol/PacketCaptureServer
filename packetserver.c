#include <stdio.h>
#include <stdlib.h>
//AF_INET, AF_INET6
#include <netinet/in.h>
//tcpヘッダ構造定義
#include <netinet/tcp.h>
//IP
#include <netinet/ip.h>
//Ether
#include <netinet/if_ether.h>
//プロトコル名とホスト名を数値アドレスに変換する定義
#include <netdb.h>
//ソケット関数とデータ構造
#include <sys/socket.h>
//インターネット操作用定義
#include <arpa/inet.h>
//デバイス制御
#include <sys/ioctl.h>
//ネットワークインターフェース構造体定義
#include <net/if.h>
//Etherフレーム
#include <net/ethernet.h>
//パケット定義
#include <netpacket/packet.h>

//受信関数定義
void recv_packet();
//Ethernetヘッダーキャプチャ用関数
int etherdump();
//ソケット
int sock;

int main(int argc, char *argv[]){
	//デバイスの設定や情報等
	struct ifreq ifr;
	//if番号とかアドレス長とか
	struct packet_mreq mreq;

	if(argc != 2){
		printf("Usage: %s <targetNIC>\n", argv[0]);
		exit(1);
	}
	//ソケット作成
	if((sock = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_ALL))) < 0){
		perror("Failed to make socket");
		exit(1);
	}
	//インターフェース番号を構造体にぶち込む
	strcpy(ifr.ifr_name, argv[1]);
	//interface index取得
	if(ioctl(sock, SIOCGIFINDEX, &ifr) < 0){
		perror("Failed to get INDEX");
		exit(1);
	}
	//mreqのサイズでメモリ確保
	memset(&mreq,0,sizeof(mreq));
	//自インターフェース以外のパケットも受信できるようにする
	mreq.mr_type = PACKET_MR_PROMISC;
	mreq.mr_ifindex = ifr.ifr_ifindex;
	if((setsockopt(sock, SOL_PACKET, PACKET_ADD_MEMBERSHIP, (void *)&mreq, sizeof(mreq))) < 0){
		perror("Failed to set socketopt");
		exit(1);
	}

	//受信関数呼び出し
	recv_packet();
}

void recv_packet(){
	/*サーバーコード*/
	//ポート
	char *port = "25566";
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
	//バインド
	if (bind(sockfd, ai->ai_addr, ai->ai_addrlen) < 0) {
		perror("Failed to bind");
		exit(1);
	}
	//listen
	if (listen(sockfd, 1) == -1){
		perror("Failed to listen");
	}
	struct sockaddr_storage client_addr;
	unsigned int address_size = sizeof(client_addr);
	
	while(1){	
	//accept
	int new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &address_size);
	if (new_sockfd == -1) {
		perror("Accept error");
	}
	/*パケット受信コード*/
	//構造体サイズ用変数
	int rsin_size, count;
	//ソケット・接続先IP・ポート情報保持用構造体
	struct sockaddr_in rsin;
	struct in_addr insaddr, indaddr;
	//ファイルディスクリプタ監視
	fd_set fds;

	//バッファに構造体ぶち込み
	struct buf
	{
		struct iphdr ip;
		struct tcphdr tcp;
		unsigned char blah[65535];
	} buf;
	rsin_size = sizeof(rsin);
	//ファイルディスクリプタ監視
	FD_ZERO(&fds);
	FD_SET(sock, &fds);

	for(count = 0 ;; count++){
		//selectしてready待ち
		if(select(sock + 1, &fds, NULL, NULL, NULL) < 0){
			perror("Failed to select");
			exit(1);
		}
		//fdの読み込みデータがあるか
		if(FD_ISSET(sock, &fds)){
			//受信
			if(recvfrom(sock, &buf, sizeof(buf), 0, (struct sockaddr *)&rsin, &rsin_size) < 0){
				perror("Failed recvfrom");
			}
			//TCP以外はほっとく
			if(buf.ip.protocol != IPPROTO_TCP)
				continue;
			insaddr.s_addr = buf.ip.saddr;
			indaddr.s_addr = buf.ip.daddr;
			//書き出しますよ
			printf("Packet number : %d\n", count);
			printf("----Ether Header-----------------\n");
			//etherdump();
			//etherヘッダ構造体
			struct ether_header *eth;
			//ヘッダ長さの分のバッファ
			char buf2[ETHER_MAX_LEN] = {0};
			//macアドレス用
			char smac[20], dmac[20];
			//ソケット用
			int sockfd2 = 0, len = 0, protocol = 0;

			//ソケット作成
			sockfd2 = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
			if(sockfd2 < 0){
				perror("socket error");
				return -1;
			}
			//受信
			len = recv(sockfd2, buf2, sizeof(buf2), 0);
			if(len < 0){
				perror("recv error");
				return -1;
			}
			eth = (struct ether_header *)buf2;
			//文字配列に格納して出力
			sprintf(dmac, "%02x:%02x:%02x,%02x:%02x:%02x",
				eth->ether_dhost[0], eth->ether_dhost[1],
				eth->ether_dhost[2], eth->ether_dhost[3],
				eth->ether_dhost[4], eth->ether_dhost[5]);
			sprintf(smac, "%02x:%02x:%02x,%02x:%02x:%02x",
				eth->ether_shost[0], eth->ether_shost[1],
				eth->ether_shost[2], eth->ether_shost[3],
				eth->ether_shost[4], eth->ether_shost[5]);
			protocol = ntohs(eth->ether_type);
			printf("src MAC     : %s\n",smac);
			printf("dst MAC     : %s\n",dmac);
			printf("protocol    : %04x\n",protocol);//4ケタ出力
			printf("size        : %dbyte\n",len);
			close(sockfd2);

			printf("----IP Header--------------------\n");
			printf("version     : %u\n",buf.ip.version);
			printf("ihl         : %u\n",buf.ip.ihl);
			printf("tos         : %u\n",buf.ip.tos);
			printf("tot length  : %u\n",ntohs(buf.ip.tot_len));
			printf("id          : %u\n",ntohs(buf.ip.id));
			printf("frag_off    : %u\n",ntohs(buf.ip.frag_off) & 8191);
			printf("ttl         : %u\n",buf.ip.ttl);
			printf("protocol    : %u\n",buf.ip.protocol);
			printf("check       : 0x%x\n",ntohs(buf.ip.check));
			printf("saddr       : %s\n",inet_ntoa(insaddr));
			printf("daddr       : %s\n",inet_ntoa(indaddr));
			printf("----TCP Header-------------------\n");
			printf("source port : %u\n",ntohs(buf.tcp.source));
			printf("dest port   : %u\n",ntohs(buf.tcp.dest));
			printf("sequence    : %u\n",ntohl(buf.tcp.seq));
			printf("ack seq     : %u\n",ntohl(buf.tcp.ack_seq));
			printf("frags       :");
			buf.tcp.fin ? printf(" FIN") : 0 ;
			buf.tcp.syn ? printf(" SYN") : 0 ;
			buf.tcp.rst ? printf(" RST") : 0 ;
			buf.tcp.psh ? printf(" PSH") : 0 ;
			buf.tcp.ack ? printf(" ACK") : 0 ;
			buf.tcp.urg ? printf(" URG") : 0 ;
			printf("\n");
			printf("window      : %u\n",ntohs(buf.tcp.window));
			printf("check       : 0x%x\n",ntohs(buf.tcp.check));
			printf("urt_ptr     : %u\n\n",buf.tcp.urg_ptr);
			//send client
			char msg[1024];
			sprintf(msg,"\n\nPacket number : %d\n----Ether Header-----------------\nsrc MAC     : %s\ndst MAC     : %s\nprotocol    : %04x\nsize        : %dbyte\n----IP Header--------------------\nversion     : %u\nihl         : %u\ntos         : %u\ntot length  : %u\nid          : %u\nfrag_off    : %u\nttl         : %u\nprotocol    : %u\ncheck       : 0x%x\nsaddr       : %s\ndaddr       : %s\n----TCP Header-------------------\nsource port : %u\ndest port   : %u\nsequence    : %u\nack seq     : %u\nfrags       :",count,smac,dmac,protocol,len,buf.ip.version,buf.ip.ihl,buf.ip.tos,ntohs(buf.ip.tot_len),ntohs(buf.ip.id),ntohs(buf.ip.frag_off) & 8191,buf.ip.ttl,buf.ip.protocol,ntohs(buf.ip.check),inet_ntoa(insaddr),inet_ntoa(indaddr),ntohs(buf.tcp.source),ntohs(buf.tcp.dest),ntohl(buf.tcp.seq),ntohl(buf.tcp.ack_seq));
			write(new_sockfd, msg, strlen(msg));
			buf.tcp.fin ? strcat(msg," FIN") : 0 ;
			buf.tcp.syn ? strcat(msg," SYN") : 0 ;
			buf.tcp.rst ? strcat(msg," RST") : 0 ;
			buf.tcp.psh ? strcat(msg," PSH") : 0 ;
			buf.tcp.ack ? strcat(msg," ACK") : 0 ;
			buf.tcp.urg ? strcat(msg," URG") : 0 ;
			write(new_sockfd, msg, strlen(msg));
			sprintf(msg,"\nwindow      : %u\ncheck       : 0x%x\nurt_ptr     : %u\n",ntohs(buf.tcp.window),ntohs(buf.tcp.check),buf.tcp.urg_ptr);
			write(new_sockfd, msg, strlen(msg));
			sleep(1);
		}
	}
	close(new_sockfd);
	}//END of while
}
