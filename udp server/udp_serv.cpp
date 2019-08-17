#include<iostream>
#include<cstring>
#include<cstdio>

#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<errno.h>
#include<signal.h>
#include<fcntl.h>
#include<assert.h>

using namespace std;

void sig_handler(int signo){
	if(signo == SIGINT){
		cout<<endl<<"检测到CTRL+C中断，服务器关闭"<<endl;
		exit(0);
	}
}

void show_info(struct sockaddr_in *addr){
	char ip[16];
	bzero(&ip, 0);
	int port = ntohs(addr->sin_port);
	inet_ntop(AF_INET, &addr->sin_addr.s_addr, ip, sizeof(ip));
	cout<<"IP:Port -> "<< ip <<" : "<< port;
}

void handle_connection(int fd){
	struct sockaddr_in cliaddr;
	bzero(&cliaddr, sizeof(cliaddr));
	socklen_t clilen = sizeof(cliaddr);
	char buf[1024];
	bzero(&buf, sizeof(buf));
	int size = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr*)&cliaddr, &clilen);
	if(size < 0){
		cerr<<"recvfrom err"<<endl;
		exit(1);
	}
	else{
		show_info(&cliaddr);
		cout<<"发来"<<buf<<endl;
		long t = time(0);
		char *ptr = ctime(&t);
		size_t n = strlen(ptr) * sizeof(char);
		int sentsize = sendto(fd, ptr, n, 0, (struct sockaddr*)&cliaddr, clilen);
		if(sentsize < 0){
			cerr<<"sentto err"<<endl;
		}
	}
}

int main(int argc, char*argv[])
{
	signal(SIGINT, sig_handler);
	
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	assert(sockfd > 0);
	
	int flag = 1;
	setsockopt(AF_INET, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(int));
	
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port =  htons(atoi(argv[1]));
	servaddr.sin_addr.s_addr = INADDR_ANY;
	
	if( (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) < 0){
		cerr<<"bind err"<<endl;
		exit(1);
	}
	
	cout<<"waiting for connection..."<<endl;
	while(1){
		handle_connection(sockfd);
	}
	close(sockfd);
	return 0;
}