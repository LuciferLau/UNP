#include<iostream>
#include<pthread.h>
#include<unistd.h>
#include<cstring>
#include<cstdio>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<errno.h>
#include<signal.h>
#include<fcntl.h>

#define MAX_EV 100
#define LISTENQ 10
#define BUFSIZE 1024

using namespace std;

void sigint_handler(int signo){
	if(signo == SIGINT){
		cout<<endl<<"检测到CTRL+C中断，服务器关闭"<<endl;
		exit(0);
	}
}

int setNonBlock(int fd){
	int ret = fcntl(fd, F_GETFL);
	if((fcntl(fd, F_SETFL, ret|O_NONBLOCK)) < 0){
		cerr<<"setNonBlock err"<<endl;
		return -1;
	}
	return 0;
}

int evADD(int efd, int fd, struct epoll_event* ev){
	int ret;
	if((ret = epoll_ctl(efd, EPOLL_CTL_ADD, fd, ev)) < 0){
		cerr<<"ev add failed"<<endl;
		return -1;
	}
	return 0;
}

int evDEL(int efd, int fd, struct epoll_event* ev){
	int ret;
	if((ret = epoll_ctl(efd, EPOLL_CTL_DEL, fd, ev)) < 0){
		cerr<<"ev delete failed"<<endl;
		return -1;
	}
	return 0;
}

int evMOD(int efd, int fd, struct epoll_event* ev){
	int ret;
	if((ret = epoll_ctl(efd, EPOLL_CTL_MOD, fd, ev)) < 0){
		cerr<<"ev modify failed"<<endl;
		exit(1);
	}
}

void showInfo(int fd){
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	getpeername(fd, (struct sockaddr*)&addr, &addrlen);
	char ip[16];
	bzero(&ip, 0);
	int port = ntohs(addr.sin_port);
	inet_ntop(AF_INET, &addr.sin_addr.s_addr, ip, sizeof(ip));
	cout<<"IP地址是："<< ip << ends <<"端口号是："<< port ;
}

int main(int argc, char* argv[])
{
	//参数检测
	if(argc < 2){
		cerr<<"参数个数出错"<<endl;
		exit(1);
	}
	
	//信号处理函数
	signal(SIGINT,sigint_handler);
	
	//变量及结构体定义
	struct epoll_event ev, events[MAX_EV];
	int listenfd ,connfd, sockfd, epollfd;
	
	//socket初始化
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	//设置为非阻塞套接字
	setNonBlock(listenfd);
	//设置地址可以重复绑定
	int flag = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(int));
	
	//服务器地址初始化
	struct sockaddr_in serveraddr;
	bzero(&serveraddr, 0);
	socklen_t serveraddr_len = sizeof(serveraddr);
	serveraddr.sin_family = AF_INET;
	//inet_pton(AF_INET, &argv[1], &serveraddr.sin_addr.s_addr, sizeof(serveraddr));
	serveraddr.sin_port = htons(atoi(argv[1]));
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//bind & listen
	if((bind(listenfd, (struct sockaddr*)&serveraddr, serveraddr_len)) < 0){
		cerr<<"bind err"<<endl;
		exit(1);
	}
	if((listen(listenfd, LISTENQ)) < 0){
		cerr<<"listen err"<<endl;
		exit(0);
	}
	
	//epoll初始化
	ev.events = EPOLLIN | EPOLLET;//使用ET边缘触发，非阻塞
	ev.data.fd = listenfd;
	epollfd = epoll_create(5);//监听5个句柄
	epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);
	
	int n, nready;
	for(;;){
		nready = epoll_wait(epollfd, events, MAX_EV, 5000);
		if(nready == 0)
			cout<<"time out"<<endl;
		for(n = 0; n < nready; ++n){
			if(events[n].data.fd == listenfd){//监听套接口有新的连接
				if((connfd = accept(listenfd, (struct sockaddr*)&serveraddr, &serveraddr_len)) < 0){
					cerr<<"accept err"<<endl;
					exit(1);
				}
				//将新来的套接字变为可读状态，加入红黑树中，并设置为非阻塞套接字
				setNonBlock(connfd);
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = connfd;
				evADD(epollfd, connfd, &ev);
			}
			else{
				char buf[BUFSIZE];
				bzero(&buf, 0);
				int size;
				if(events[n].events & EPOLLIN){//有其它套接字可以读					
					if((sockfd = events[n].data.fd) < 0)
						continue;					
					if((size = read(sockfd, buf, sizeof(buf))) < 0){//发生读错误
						cerr<<"read err"<<endl;
						exit(1);
					}
					if(size == 0){//客户端断开连接
						showInfo(sockfd);cout<<"断开连接"<<endl;
						close(sockfd);
						sockfd = -1;
						continue;
					}
					//正常情况
					cout<<"接收到客户端";
					showInfo(sockfd);
					cout<<"的"<<size<<"字节的消息，消息为："<<buf<<endl;
					//套接字读完了，变为可写状态。
					ev.events = EPOLLOUT | EPOLLET;
					ev.data.fd = sockfd;
					evMOD(epollfd, sockfd, &ev);
				}
				if(events[n].events & EPOLLOUT){//有其它套接字可以写	
					if((sockfd = events[n].data.fd) < 0)
						continue;
					if((size = write(sockfd, buf, sizeof(buf))) < 0){//发生写错误
						cerr<<"write err"<<endl;
						exit(1);
					}
					cout<<"向客户端发送了"<<size<<"字节数据："<<buf<<endl;
					//套接字写完了，变回可读状态。
					ev.events = EPOLLIN | EPOLLET;
					ev.data.fd = sockfd;
					evMOD(epollfd, sockfd, &ev);
					bzero(&buf, 0);
				}
			}
			
		}
	}
	close(listenfd);
	return 0;
}