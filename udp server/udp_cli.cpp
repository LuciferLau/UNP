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
		cout<<endl<<"检测到CTRL+C中断，客户端关闭"<<endl;
		exit(0);
	}
}

int main(int argc, char*argv[])
{
	signal(SIGINT, sig_handler);
	
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	assert(sockfd > 0);
	
	struct sockaddr_in servaddr;
	socklen_t servlen = sizeof(servaddr);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port =  htons(atoi(argv[1]));
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	while(1){
		char buf[1024];
		bzero(&buf, sizeof(buf));
		cin>>buf;
		int sendsize = sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&servaddr, servlen);
		
		if(sendsize < 0){
			cerr<<"sentto err"<<endl;
		}
		else{
			cout<<"successfully send!"<<endl;
			bzero(&buf, sizeof(buf));
			int recvsize = recv(sockfd, buf, sizeof(buf), 0);
			//因为UDP是无连接的，所以不需要判断是否为0
			if(recvsize < 0){
				cerr<<"recv err"<<endl;
				exit(1);
			}
			else{
				cout<<"recv msg: "<<buf<<endl;
			}
		}
	}
	close(sockfd);
	return 0;
}