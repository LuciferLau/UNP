#include<iostream>
#include<vector>
#include<pthread.h>
#include<unistd.h>
#include<assert.h>
#include<signal.h>
#include<time.h>
#include<sys/select.h>
#include<arpa/inet.h>

#include<memory>
#include<cstring>
#include<cstdlib>
#include<cstdio>


#ifndef LISTENQ
#define LISTENQ 5 //监听队列长度
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
	cout<<"端口号："<<port<<" 的连接已断开。"<<endl;
}

int main(int argc,char *argv[]){
	if(argc<2)
		return -1;
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(listenfd > 0);
	
	struct sockaddr_in serveraddr;
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serveraddr.sin_port = htons(atoi(argv[1]));
	serveraddr.sin_family = AF_INET;
	socklen_t serveraddr_len = sizeof(serveraddr);
	
	int flag = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(int));
	
	if((bind(listenfd, (struct sockaddr*)&serveraddr, serveraddr_len)) < 0){
		cerr<<"bind err"<<endl;
		exit(1);
	}
	if((listen(listenfd, LISTENQ)) < 0){
		cerr<<"listen err"<<endl;
		exit(0);
	}
	
	fd_set allset,rset;//rset每次都要清零，所以用allset保存初始状态
	FD_ZERO(&allset);
	FD_ZERO(&rset);
	FD_SET(listenfd, &allset);
	struct sockaddr_in clientaddr;
	socklen_t len = sizeof(clientaddr);
	int connfd, maxi, maxfd, nready;
	int clients[FD_SETSIZE];
	for(int i=0; i<FD_SETSIZE; ++i)
		clients[i] = -1;
	maxfd = listenfd;
	cout<<"waiting for conection..."<<endl;
	while(1){
		rset = allset;
		nready = select(maxfd+1, &rset, NULL, NULL, NULL);
		if(nready < 0){
			cerr<<"select error"<<endl;
			break;
		}
		if(nready == 0)
			continue;
		//新连接到来
		if(FD_ISSET(listenfd, &rset)){
			if( (connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &len)) < 0){
				cerr<<"accept err"<<endl;
				continue;
			}
			else {
				get_cliname(connfd);
				FD_SET(connfd, &allset);
				int i=0;
				for(;i<FD_SETSIZE; ++i){
					if(clients[i] < 0){
						clients[i] = connfd;
						break;
					}
				}
				if(i == FD_SETSIZE)
					exit(1);
				if(i > maxi)
					maxi = i;
				if(connfd > maxfd)
					maxfd = connfd;
				if(--nready <= 0)
					continue;
			}
		}
		//处理上面接收的连接		
		for(int i=0; i<=maxi; ++i){
			int fd = clients[i];
			if(fd < 0)
				continue;
			char buf[1024];
			bzero(&buf, sizeof(buf));
			if(FD_ISSET(fd, &rset)){
				int size = read(fd, buf, sizeof(buf));
				if(size < 0){
					cerr<<"read err"<<endl;
					continue;
				}
				else if(size == 0){
					quit_info(fd);
					FD_CLR(fd, &allset);
					close(fd);
					clients[i] = -1;
				}
				else{
					cout<<"receive msg: "<<buf<<endl;
					write(fd, buf, sizeof(buf));
					bzero(&buf, sizeof(buf));
				}
				if(--nready <= 0)
					break;
			}
		}
	}
	close(listenfd);
	return 0;
}