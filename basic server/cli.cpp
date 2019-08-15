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
	
	//服务器地址结构初始化
	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(atoi(argv[1]));
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");//只连接本地客户端
	
	//connect
	int ret;
	if( (ret = connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr))) < 0){
		cerr<<"conn err"<<endl;
		exit(1);
	}
	char buf[1024];
	bzero(&buf,sizeof(buf));
	while(1){
		//写数据
		cin>>buf;
		if( buf[0] == 'q' ){
			close(sockfd);
			exit(1);
		}
		int n;
		if( (n = write(sockfd, buf, sizeof(buf))) < 0){
			cerr<<"write err"<<endl;
			exit(1);
		}
		else{
			cout<<"发送成功"<<endl;
			bzero(&buf,sizeof(buf));
		}
		//读数据
		if( (n = read(sockfd, buf, sizeof(buf))) < 0){
			cerr<<"read drr"<<endl;
			exit(1);
		}
		else{
			cout<<buf<<endl;
			bzero(&buf, sizeof(buf));
		}	
	}
	close(sockfd);
	return 0 ;
}