#include<iostream>
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

void do_job(int connfd){
	int ret;
	char buf[1024];
	bzero(&buf, sizeof(buf));
	for(;;){
		if( (ret = read(connfd, buf, sizeof(buf))) < 0){
			cerr<<"read err"<<endl;
		}
		else if(ret == 0){
			cout<<"客户端连接已断开"<<endl;
			close(connfd);
			break;
		}
		else{
			cout<<"接收到信息："<<buf<<endl;			
			write(connfd, buf, sizeof(buf));
			bzero(&buf,sizeof(buf));
		}
	}
}

int main(int argc, char *argv[])
{
	//参数设置
	if(argc < 1){
		cerr<<"缺少端口参数"<<endl;
		exit(1);
	}
	
	//服务器套接字
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(sockfd != 0);
	
	//信号处理函数
	signal(SIGINT,sig_handler);
	
	//SOCKET设置
	int opt_value = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt_value, sizeof(int));
	
	//服务器地址结构初始化
	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(atoi(argv[1]));
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");//只连接本地客户端
	
	//bind & listen
	if(bind(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
		cerr<<"bind err"<<endl;
	if(listen(sockfd, 5) < 0)
		cerr<<"listen err"<<endl;
	
	pid_t pid;//进程PID
	int connfd;//连接的套接字
	socklen_t len = sizeof(serveraddr);
	char buf[1024];
	bzero(&buf,sizeof(buf));
	
	while(1){
		if( (connfd = accept(sockfd,(struct sockaddr*)&serveraddr, &len)) < 0){
			cerr<<"accept err"<<endl;
			continue;
		}		
		if( (pid = fork()) < 0){
			cerr<<"fork err"<<endl;
			exit(1);
		}
		else if(pid == 0){//子进程
			close(sockfd);
			do_job(connfd);	
			exit(0);
		}
		else{//父进程
			close(connfd);
			continue;//啥也不干
		}
	}
	close(sockfd);
	return 0 ;
}