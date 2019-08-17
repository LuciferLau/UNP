#include<iostream>
#include<pthread.h>
#include<unistd.h>
#include<assert.h>
#include<signal.h>
#include<time.h>
#include<poll.h>
#include<arpa/inet.h>

#include<memory>
#include<cstring>
#include<cstdlib>
#include<cstdio>

#ifndef LISTENQ
#define LISTENQ 5 //监听队列长度
#endif

#ifndef POLLQ
#define POLLQ 512 //监听队列长度
#endif

using namespace std;

void sig_handler(int signo){
	if(signo == SIGINT){
		cerr<<" Checked CTRL+C Server Closed."<<endl;
		exit(1);
	}
}

void get_cliname(int fd){
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	getpeername(fd, (struct sockaddr*)&addr, &len);
	char ip[16];
	bzero(ip, sizeof(ip));
	int port = ntohs(addr.sin_port);
	inet_ntop(AF_INET, &addr.sin_addr.s_addr, ip, sizeof(ip));
	cout<<"主机地址："<<ip<<" 端口号："<<port<<" 已连接。"<<endl;
}

void quit_info(int fd){
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	getpeername(fd, (struct sockaddr*)&addr, &len);
	int port = ntohs(addr.sin_port);
	cout<<"端口号："<<port<<"的主机已断开。"<<endl;
}

int main(int argc, char*argv[]){
	if(argc < 2){
		cerr<<"wrong nums of argument"<<endl;
		exit(1);
	}
	
	signal(SIGINT, sig_handler);
	
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(listenfd > 0);
	
	int flag = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(int));
	
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_port = htons(atoi(argv[1]));
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_family = AF_INET;
	
	if( (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) < 0)
		exit(1);
	if( (listen(listenfd, LISTENQ)) < 0)
		exit(1);
	
	//poll part
	struct pollfd clients[POLLQ];
	int nready, connfd, fd;
	int maxi = 1;
	struct sockaddr_in cliaddr;
	socklen_t clilen = sizeof(cliaddr);
	char buf[1024];
	bzero(&buf, sizeof(buf));
	//init pollfd
	clients[0].fd = listenfd;
	clients[0].events = POLLIN;
	for(int i=1; i<POLLQ; ++i)
		clients[i].fd = -1;
	cout<<"waiting for connection..."<<endl;
	while(1){
		int usec = 5000;
		nready = poll(clients, maxi, -1);
		if(nready < 0){
			cerr<<"poll err"<<endl;
			continue;
		}
		if(nready == 0)
			continue;		
		//new connection
		if(clients[0].revents & POLLIN){
			connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
			assert(connfd > 0);
			int i=1;
			for(;i<POLLQ; ++i){
				if(clients[i].fd<0){					
					clients[i].fd = connfd;
					clients[i].events = POLLIN;
					break;
				}
			}
			if(i == POLLQ)
				cerr<<"POLL QUEUE IS FULL"<<endl;
			if(i > maxi)
				maxi = i;
			if(--nready <= 0)
				continue;
		}
		//process these fds
		for(int i=1; i<maxi; ++i){		
			fd = clients[i].fd;
			if(fd < 0)
				continue;
			if(clients[i].revents & POLLIN){
				int size = read(fd, buf, sizeof(buf));
				if(size < 0)
					cerr<<"read err"<<endl;
				else if(size == 0){
					quit_info(fd);
					close(fd);
					clients[i].fd = -1;
				}
				else{
					cout<<"receive msg: "<<buf<<endl;
					write(fd, buf, sizeof(buf));
					bzero(&buf, sizeof(buf));
				}
				if(--nready <=0)
					break;
			}
		}
	}
	
	close(listenfd);
	return 0;
}