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
		printf("%s interface in use\n", argv[0]);
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
			etherdump();
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
			sleep(1);
		}
	}
}

int etherdump(){
	//etherヘッダ構造体
	struct ether_header *eth;
	//ヘッダ長さの分のバッファ
	char buf[ETHER_MAX_LEN] = {0};
	//macアドレス用
	char smac[20], dmac[20];
	//ソケット用
	int sockfd = 0, len = 0, protocol = 0;

	//ソケット作成
	sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if(sockfd < 0){
		perror("socket error");
		return -1;
	}
	//受信
	len = recv(sockfd, buf, sizeof(buf), 0);
	if(len < 0){
		perror("recv error");
		return -1;
	}
	eth = (struct ether_header *)buf;
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
	close(sockfd);
	return 0;
}