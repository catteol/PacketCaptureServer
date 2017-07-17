#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
//#include <malloc.h>
#include <netdb.h>
#include <stdio.h>

#define ERR_RET(mes,ret){fputs(mes,stderr);return ret;}

#define FD_STDIN 0
#define BUF_SIZE 0xFF

int gethost(char*pStr, struct hostent*pHost)
{
    struct hostent*ph=gethostbyaddr(pStr, 4, AF_INET);
    if(!ph)
    {
	//アドレスでないのでホスト名として扱う
	ph=gethostbyname(pStr);
	if(!ph)
	    return 0; //失敗
    }
    memcpy(pHost, ph, sizeof(struct hostent));
    return 1; //成功
}

int recvMes(int sock)
{
    int len;
    char buf[BUF_SIZE+1];
    while((len=recv(sock, buf, BUF_SIZE, 0))>0)
    {
	buf[len]=0;
	fputs(buf, stdout);
       	if(strchr(buf, '\n'))
	   return 0;
    }
    close(sock);
    return 1;
}

int sendMes(char*pUser, int sock)
{
    char buf[BUF_SIZE+1];
    int len;

    fgets(buf, BUF_SIZE, stdin);

    //終了かチェック
    if(!strcmp(buf, "QUIT\n"))
	return 1;

    send(sock, pUser, strlen(pUser), 0);
    send(sock, "\t\t: ", 4, 0);

    while(!feof(stdin))
    {
	send(sock, buf, strlen(buf), 0);
	if(strchr(buf, '\n'))
	    return 0;
	fgets(buf, BUF_SIZE, stdin);
    }
    return 1;
}

int check(char*pUser, int sock)
{
    struct sockaddr_in addr;
    int addrSize=sizeof(addr);
    int i, ret, newSock;
    struct fd_set fds;

    FD_ZERO(&fds);
    FD_SET(FD_STDIN, &fds);
    FD_SET(sock, &fds);

    ret=select(FD_SETSIZE, &fds, NULL, NULL, NULL);
    if(ret<0)
	ERR_RET("select error\n", 1);

    if(FD_ISSET(FD_STDIN, &fds))
    {
	if(sendMes(pUser, sock))
	{
	    return 1;
	}
    }

    if(FD_ISSET(sock, &fds))
    {
	if(recvMes(sock))
	{
	    puts("____disconnect server____");
	    return 1;
	}
    }

    return 0;
}

int loop(char*pUser, int sock)
{
    while(!check(pUser, sock))
    {

    }
    return 0;
}

int start(char*pUser, char*pHost, int Port)
{
    int sock;
    struct hostent host;
    struct sockaddr_in addr;

    if(!gethost(pHost, &host))
    {
	fputs("invalid hostname\n", stderr);
	return 1;
    }
    sock=socket(AF_INET, SOCK_STREAM, 0);
    if(sock<0)
    {
	fputs("socket error\n", stderr);
	return 1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_port=htons(Port);
    addr.sin_addr=*(struct in_addr*)*host.h_addr_list;

    if(connect(sock, (struct sockaddr*)&addr
	       , sizeof(struct sockaddr_in))!=0)
    {
	fputs("connect error\n", stderr);
	return 1;
    }

    loop(pUser, sock);
    close(sock);
    return 0;
}

int main(int argc, char**argv)
{
    int port;

    if(argc<4)
    {
	fputs("usage:./client user host port\n", stderr);
	return 1;
    }

    port=atoi(argv[3]);
    if(port==0)
    {
	fputs("invalid port\n", stderr);
	return 1;
    }
    start(argv[1], argv[2], port);
    return 0;
}
