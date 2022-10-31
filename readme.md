# WebServer

用C++实现的Linux高性能服务器，经过webbenchh压力测试可以实现上万的QPS

## 功能

* 使用正则解析HTTP get/post 请求报文,string_view存储结果避免拷贝
* 使用队列避免排序常数O(1)实现的定时器，支持HTTP长连接以及超时断开
* 采用epoll ET/LT IO复用技术
* 实现线程池，主线程负责连接和关闭，工作线程负责收发网络数据，实现Reactor的IO模式
* 线程池对任务处理是抽象层面，通过主线程的epoll的EPOLLONESHOT事件模式，保证处理请求是有序的。
* 实现数据库连接池，提高对数据库操作的性能,通过访问数据库操作实现用户注册、登录功能
* 双缓冲区异步日志系统，记录服务器运行状态，使其对性能的影响只有不到20%
* 对访问资源建立缓冲区，采用LRU策略，减少读取资源的IO操作,但内存的释放由任务流程决定，保证线程安全

## 模板

* log.h  采用单例模式,通用的可以安全关闭的双缓冲异步日志
* safequeue.h 线程安全的队列
* simvector.h 避免构造函数的，以及分配器的简单数据类型的vector,自已存在一套内存增长的策略,并提供char类型特化,
* threadpool.h 可以返回任务处理结果的线程池，并极力降低锁本身占用的资源
* LRUCache.h 一个LRU策略的模板类，可以定量每个节点所占的大小

## 环境要求

* Linux
* C++17
* MySql

## 项目启动

需要先配置好对应的数据库

```mysql
// 建立yourdb库
create database yourdb;

// 创建user表
USE yourdb;e
CREATE TABLE user(
    username char(50) NULL,
    password char(50) NULL
)ENGINE=InnoDB;

// 添加数据
INSERT INTO user(username, passwd) VALUES('name', 'passwd');
```

```bash
make
./bin/server p 端口 t 线程数 l 日志开关 r 日志等级 s 连接池
```

## 单元测试

```bash
cd test
make
./test
```

## 压力测试

```bash
./webbench-1.5/webbench -c 10000 -t 10 http://ip:port/
```

* QPS 21000~22000

## 参考

[TinyWebServer](https://github.com/Yoka416/TinyWebServer)
[LINUX手册](https://linux.die.net/)
[C++手册](https://zh.cppreference.com/w/cpp)
[C++线程池](https://wangpengcheng.github.io/2019/05/17/cplusplus_theadpool/)
[HTTP协议](https://zh.wikipedia.org/wiki/%E8%B6%85%E6%96%87%E6%9C%AC%E4%BC%A0%E8%BE%93%E5%8D%8F%E8%AE%AE)
