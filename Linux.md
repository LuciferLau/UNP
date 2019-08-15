# 网络编程相关Linux知识

    • Linux的I/O模型介绍以及同步异步阻塞非阻塞的区别（超级重要）
同步IO：阻塞I/O，非阻塞I/O，信号I/O，I/O复用。  
异步IO：异步I/O。  
阻塞：用户进访问数据时，阻塞在当前调用，等待内核完成IO操作。  
非阻塞：用户访问数据时，立刻获得一个状态值，然后进程继续向下执行。  

    •	文件系统的理解（EXT4，XFS，BTRFS）tobedone
    
    •	文件处理grep,awk,sed这三个命令必知必会 tobedone
grep：文本过滤器；awk：报告生成器，支持正则表达式和简单编程；sed：流编辑器。  

•	IO复用的三种方法（select,poll,epoll）深入理解，包括三者区别，内部原理实现？tobedone 

•	Epoll的ET模式和LT模式（ET的非阻塞）  
LT（level trigger）水平触发：程序可以不马上处理事件，下次调用epoll_wait时再次通知程序这个事件。  
ET（edge trigger）边缘触发：程序必须立刻处理事件，否则下次调用epoll_wait时不通知程序这个世事件。  
ET效率比LT高，且必须使用非阻塞套接口避免一个fd进行阻塞I/O饿死其它fds。  

•	查询进程占用CPU的命令（注意要了解到used，buf，cache代表意义）  
top，默认刷新时间是2s，buf是内存和硬盘使用的缓冲，cache是CPU和内存使用的高速缓冲，mem中used代表使用中的内存，swap中used代表使用中的交换区。  
buffer是即将要被写入磁盘的，而cache是被从磁盘中读出来的。  

•	linux的其他常见命令（kill，find，cp等等）  
kill：根据pid终止进程，可以按不同选项发送信号给进程。  
find：从指定目录递归的查找指定文件。   
cp：将文件拷贝到某一位置。  

•	shell脚本用法（只会hello world）  
''  
!# bim/bash
echo 'hello world'
''  

•	硬连接和软连接的区别  
硬链接创建文件的另一个实体，但是使用相同的iNode，实际上是文件的另一个别名。  
软链接类似Windows系统下的快捷方式，里面包含着链接文件的绝对路径，如果链接文件被删除，就无法访问对应的绝对路径，但它有自己的iNode。  
如图可见硬链接和文件testln的iNode是相同的，而软链接的不一样。  
 
•	文件权限怎么看（rwx）  
ls -l 可以看到文件的权限，r读，w写，x执行，通过chmod更改权限，选项u用户，g用户组，o其他用户，+表示添加，-表示删除，也可以用0~7数字表示权限。  

•	文件的三种时间（mtime, atime，ctime），分别在什么时候会改变  
mtime: 文件被改rwx的时间；atime: 文件被访问的时间；ctime: 文件被编辑的时间。  

•	Linux监控网络带宽的命令，查看特定进程的占用网络资源情况命令tobedone  
监控总体带宽使用：nload、bmon、slurm、bwm-ng、cbm、speedometer和netload；  
监控总体带宽使用（批量式输出）：vnstat、ifstat、dstat和collectl；  
每个套接字连接的带宽使用：iftop、iptraf、tcptrack、pktstat、netwatch和trafshow；  
每个进程的带宽使用：nethogs。  
