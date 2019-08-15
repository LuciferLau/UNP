//服务器程序

#include<iostream>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<memory>
#include<assert.h>
#include<signal.h>

using std::cout;
using std::cerr;
using std::endl;
using std::cin;

int sockfd;
int num = 0;

void sig_handle(int signo){
	if(signo == SIGINT){
		cout<<"检测到CTRL+C，服务器关闭"<<endl;
		close(sockfd);
		exit(1);
	}
}

void* job(void* arg){
	int fd = *((int*)arg);//将void*强转为int*再用*获取它的值
	//通过FD获取客户端信息：IP地址，端口号
	struct sockaddr_in clientaddr;
	socklen_t len = sizeof(clientaddr);
	getpeername(fd, (struct sockaddr*)&clientaddr, &len);
	int port = ntohs(clientaddr.sin_port);
	char ip[16];
	bzero(ip, sizeof(ip));
	inet_ntop(AF_INET, &clientaddr.sin_addr.s_addr, ip, sizeof(ip));
	
	char buf[1024];
	bzero(buf, sizeof(buf));
	for(;;){
		//从客户端读文件
		if(read(fd, buf, sizeof(buf)) > 0){
			cout<<ip<<" : "<<port<<" 说："<<buf<<endl;
			write(fd, buf, sizeof(buf));
			bzero(buf, sizeof(buf));	
		}
		else if(read(fd, buf, sizeof(buf)) == 0){
			cout<<"客户 "<<ip<<" : "<<port<<" 已断开"<<endl;
			close(fd);
			break;
		}		
		else{
			cerr<<"read err"<<endl;
			close(fd);
			exit(1);
		}
	}
}

int main(int argc, char*argv[])
{
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(sockfd != 0);
	
	signal(SIGINT,sig_handle);
	
	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(atoi(argv[1]));//命令行二参数为目的端口号
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	
	int ret = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(ret));
	if( bind(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
		cerr<<"bind err"<<endl;;
	
	if( listen(sockfd, 5) < 0)
		cerr<<"listen err"<<endl;;
	
	int connfd;//连接fd
	pthread_t tid;//线程id
	while(1){
		
		struct sockaddr_in clientaddr;
		socklen_t clientaddr_len = sizeof(clientaddr);
		if((connfd = accept(sockfd, (struct sockaddr*)&clientaddr, &clientaddr_len)) < 0)
			cerr<<"accept err, errno: "<<errno<<endl;				
		else{
			char ip[16];
			bzero(ip,sizeof(ip));
			int port = ntohs(clientaddr.sin_port);
			inet_ntop(AF_INET, &clientaddr.sin_addr.s_addr, ip, sizeof(ip));
			cout<<"接受到主机："<<ip<<"来自端口:"<<port<<"的连接！"<<endl;
			
			pthread_attr_t tattr;
			pthread_attr_init(&tattr);
			pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);//分离状态创建线程
			int err;
			if((err = pthread_create(&tid, &tattr, job, &connfd)) < 0)
				cerr<<"线程创建失败，errno:"<<err<<endl;
			pthread_attr_destroy(&tattr);
		}
	}
	close(sockfd);
	return 0;
}