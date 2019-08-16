# 使用select实现I/O多路复用
> 维护一个clients数组保存所有句柄，两个fd_set，一个用于保存所有句柄，另一个保存就绪的句柄。  
> 变量maxi，maxfd，nready控制循环执行。
