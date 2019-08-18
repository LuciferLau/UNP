//C++ STL
#include<iostream>
#include<cstring>
#include<cstdio>
#include<queue>
//系统
#include<unistd.h>
#include<assert.h>
#include<errno.h>
#include<signal.h>
#include<fcntl.h>
#include<pthread.h>
//网络相关
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/epoll.h>

#define MAX_EV 1024
#define LISTENQ 10
#define BUFSIZE 1024

using namespace std;

void sig_handler(int signo){
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
	cout<<"IP:Port -> "<< ip <<""<< port;
}

//变量及结构体定义
static struct epoll_event ev, events[MAX_EV];
int listenfd ,connfd, sockfd, epollfd;
	
class Task{
public:
	Task(void* (*fcn)(void*),void* args):work(fcn),arg(args){};
	void process(){
		work(arg);
	}
private:
	void* (*work)(void* arg);
	void* arg;
};

class ThreadPool{
public:
	ThreadPool(int thread_num, int max_requests): m_thread_num(thread_num), m_max_request(max_requests),
	m_stop(false), m_threads(nullptr){
		if(thread_num<=0 || max_requests<=0)
			exit(1);
		//创建thread_num个线程描述符，数组保存
		m_threads = new pthread_t[m_thread_num];
		assert(m_threads != nullptr);
		//线程分离属性
		pthread_attr_t DETACH;
		pthread_attr_init(&DETACH);
		pthread_attr_setdetachstate(&DETACH, PTHREAD_CREATE_DETACHED);
		for(int i=0; i<thread_num; ++i){
			//cout<<"正创建第 "<<i+1<<" 个线程"<<endl;
			if( pthread_create(m_threads+i, &DETACH, worker, this) != 0){
				delete[] m_threads;
				cerr<<"创建线程出错"<<endl;
				exit(1);
			}
		}
		cout<<"线程池初始化完成！"<<endl;
	}
	
	~ThreadPool(){
		delete[] m_threads;
		m_stop = true;
	}
	
	bool addTask(Task* request){
		pthread_mutex_lock(&mutex);
		//cout<<"任务队列获取锁"<<endl;
		if(m_workqueue.size() > m_max_request){
			pthread_mutex_unlock(&mutex);
			//cout<<"任务队列释放锁"<<endl;
			//cout<<"任务添加失败"<<endl;
			return false;
		}
		m_workqueue.push(request);
		pthread_mutex_unlock(&mutex);
		//cout<<"任务队列释放锁"<<endl;
		//cout<<"任务已添加，cond唤醒线程中..."<<endl;
		pthread_cond_signal(&cond);
		return true;
	}
	
private:
	static void* worker(void* arg);
	void run();
	
private:
	int m_thread_num;//线程数
	int m_max_request;//最大任务数
	pthread_t *m_threads;//线程描述符数组
	queue<Task*>m_workqueue;//任务队列
	bool m_stop;//线程池状态
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;	
};

void* ThreadPool::worker(void* arg){
	ThreadPool *pool = (ThreadPool*)arg;
	pool->run();
	return pool;
}

void ThreadPool::run(){
	while(!m_stop){
		//cout<<"线程id:"<<pthread_self()<<" 正在等待被唤醒"<<endl;
		pthread_cond_wait(&cond, &mutex);
		cout<<"id:"<<pthread_self()<<" 获取条件变量"<<endl;
		pthread_mutex_trylock(&mutex);//用lock会死锁，trylock就可以执行，为什么？tobedone
		if(errno == EBUSY)
			cerr<<"线程尝试加锁失败"<<endl;
		cout<<"当前任务队列长度为："<<m_workqueue.size()<<endl;
		if(m_workqueue.empty()){
			pthread_mutex_unlock(&mutex);
			cout<<"id:"<<pthread_self()<<" 释放mutex:m_workqueue.empty()"<<endl;
			continue;
		}
		Task* request = m_workqueue.front();
		m_workqueue.pop();
		pthread_mutex_unlock(&mutex);
		cout<<"id:"<<pthread_self()<<" 释放mutex:finish"<<endl;
		cout<<endl;
		if(!request)
			continue;
		request->process();
	}
}

void* readTask(void* arg){
	cout<<"id:"<<pthread_self()<<" 线程正在执行读取任务..."<<endl;
	int sockfd = *((int*)arg);
	char buf[BUFSIZE];
	bzero(&buf, 0);
	int size;
	assert(sockfd > 0);				
	if((size = read(sockfd, buf, sizeof(buf))) < 0){//发生读错误
		cerr<<"read err"<<endl;
		exit(1);
	}
	if(size == 0){//客户端断开连接
		showInfo(sockfd);cout<<"断开连接"<<endl;
		close(sockfd);
		sockfd = -1;
		return (void*)0;
	}
	//正常情况
	cout<<"接收到客户端";
	showInfo(sockfd);
	cout<<"的"<<size<<"字节的消息，消息为："<<buf<<endl<<endl;
	//套接字读完了，变为可写状态。
	ev.events = EPOLLOUT | EPOLLET;
	ev.data.fd = sockfd;
	evMOD(epollfd, sockfd, &ev);
	usleep(1234);//休息1234毫秒
	return (void*)0;
}

void* writeTask(void* arg){
	cout<<"id:"<<pthread_self()<<" 线程正在执行写入任务..."<<endl;
	int sockfd = *((int*)arg);
	time_t t_now = time(0);
	char *ptr = ctime(&t_now);
	size_t t_size = sizeof(char) * strlen(ptr);
	int size;
	assert(sockfd > 0);	
	if((size = send(sockfd, ptr, t_size, 0)) < 0){//发生写错误
		cerr<<"write err"<<endl;
		exit(1);
	}
	cout<<"向客户端发送了"<<t_size<<"字节数据："<<ptr<<endl<<endl;
	//套接字写完了，变回可读状态。
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = sockfd;
	evMOD(epollfd, sockfd, &ev);
	usleep(1234);//休息1234毫秒
	return (void*)0;
}

int main(int argc, char* argv[])
{
	//参数检测
	if(argc < 2){
		cerr<<"参数个数出错"<<endl;
		exit(1);
	}
	
	//信号处理函数
	signal(SIGINT,sig_handler);	
	
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
	epollfd = epoll_create(2048);//监听5个句柄
	epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);
	//线程池初始化
	ThreadPool pool(8,MAX_EV);
	cout<<"waiting for connection..."<<endl;
	for(;;){
		int nready = epoll_wait(epollfd, events, MAX_EV, 0);
		if(nready < 0){
			cerr<<"epoll_wait err"<<endl;
			exit(1);
		}
		if(nready == 0)
			continue;
		for(int n = 0; n < nready; ++n){
			if(events[n].data.fd == listenfd){//监听套接口有新的连接
				if((connfd = accept(listenfd, (struct sockaddr*)&serveraddr, &serveraddr_len)) < 0){
					cerr<<"accept err"<<endl;
					exit(1);
				}
				showInfo(connfd); cout<<"is connecting!"<<endl;
				//将新来的套接字变为可读状态，加入红黑树中，并设置为非阻塞套接字
				setNonBlock(connfd);
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = connfd;
				evADD(epollfd, connfd, &ev);
			}
			else{
				if(events[n].events & EPOLLIN){//有其它套接字可以读
					sockfd = events[n].data.fd;
					Task* r_task = new Task(readTask, &sockfd);
					pool.addTask(r_task);
				}
				if(events[n].events & EPOLLOUT){//有其它套接字可以写	
					sockfd = events[n].data.fd;
					Task* w_task = new Task(writeTask, &sockfd);
					pool.addTask(w_task);
				}
			}
			
		}
	}
	close(listenfd);
	return 0;
}



