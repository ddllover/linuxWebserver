# WebServer

用C++实现的Linux高性能服务器，经过本地webbenchh压力测试可以实现上万的QPS，并利用阿里云搭建了一个简易的网站
[测试网站](https://luklapse.top/)

## 功能

* 使用正则解析HTTP get/post 请求报文,string_view存储结果避免拷贝
* 使用队列避免排序常数O(1)实现的定时器，支持HTTP长连接以及超时断开
* 采用epoll ET/LT IO复用技术
* 实现线程池，主线程负责连接和关闭，工作线程负责收发网络数据，实现Reactor的IO模式
* 线程池对任务处理是抽象层面，通过主线程的epoll的EPOLLONESHOT事件模式，保证处理请求是有序的。
* 实现数据库连接池，提高对数据库操作的性能,通过访问数据库操作实现用户注册、登录功能
* 单例模式创建双缓冲区异步日志系统，记录服务器运行状态，使其对性能的影响只有不到20%，并可以定时刷新方式
* 对访问资源建立缓冲区，采用LRU策略，减少读取资源的IO操作,但内存的释放由任务流程决定，增加一个计数，保证线程安全

## 模板

* log.h  采用单例模式,通用的可以安全关闭的双缓冲异步日志
* safequeue.h 线程安全的队列
* simvector.h 避免构造函数的，以及分配器的简单数据类型的vector,自已存在一套内存增长的策略,并提供char类型特化,
* threadpool.h 可以返回任务处理结果的线程池，并极力降低锁本身占用的资源
* LRUCache.h 一个LRU策略的模板类，可以定量每个节点所占的大小

## 环境要求

* Linux
* C++17
* MySql: libmysqlclient-dev

## docker 项目部署

linux本地: 
```bash
git clone https://github.com/ddllover/linuxWebserver.git
cd linuxWebserver
make
```
docker:
Dockfile文件:
```Dockerfile
FROM ubuntu
RUN set -x; buildDeps='g++ libmysqlclient-dev' \  
&& apt-get update \
&& apt-get install -y $buildDeps\
&& cd /root\
&& mkdir linuxWebserver
```
docker-compose.yml文件:请修改相应的volumes
```yml
version: "3.9"
services:
  nginx:
    image: nginx
    container_name: nginx
    ports: 
      - 80:80
      - 443:443
    volumes:
      #挂载相对应文件，保存记录到本机
      - ./nginx/nginxconfig.io:/etc/nginx/nginxconfig.io
      - ./nginx/sites-available:/etc/nginx/sites-available
      - ./nginx/sites-enabled:/etc/nginx/sites-enabled
      - ./nginx/nginx.conf:/etc/nginx/nginx.conf
      - ./nginx/dhparam.pem:/etc/nginx/dhparam.pem
      #这个地方挂载你的certbot申请的证书，如果想使用其他证书请修改nginx相关的文件
      - /etc/letsencrypt:/etc/letsencrypt 
    networks:
      - web_net
    depends_on:
      - serweb
  serweb:
    image: newweb
    container_name: serweb
    expose:
      - 1234
    volumes: #挂载相对应文件，保存记录到本机
      - ../log:/log
      - ../bin/server:/root/server
      - ../resources:/root/linuxWebserver/resources
    networks:
      - web_net
    entrypoint: ["/root/server","-p","1234"]
    depends_on:
      - mysql
  mysql:
    image: mysql
    container_name: mysql
    expose: 
     - 3306
    environment:
       MYSQL_ROOT_PASSWORD: '123456'
       MYSQL_DATABASE: 'yourdb'
    volumes: #挂载相对应文件，保存记录到本机
      - ./mysql/db:/var/lib/mysql 
    networks:
      - web_net
    command: 
      - --default-authentication-plugin=mysql_native_password #解决外部无法访问的问题
networks:
  web_net:
```

若想本地运行请在mian.cpp修改mysql配置，并本地配置
```
./bin/server -p 端口 -t 线程数 -f 日志开关 -l 日志等级 -s 连接池
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

## 下一步

* 使用libcurl或者其他成熟的库，来实现一个简易的网站,支持https http 所有协议和请求
* 增加资源成本,考虑把资源存放在另一个系统,比如数据库,从而实现更加强大的功能
* 考虑一个简单可套的前端模板
* 抛弃底层的一部分和tcp相关的应用层细节，用相应的库或者反向代理实现，自己转型处理业务逻辑
* 可能的实现：资源网站,可以上传资料,可以当云盘使用

## 参考

[TinyWebServer](https://github.com/Yoka416/TinyWebServer)

[LINUX手册](https://linux.die.net/)

[C++手册](https://zh.cppreference.com/w/cpp)

[C++线程池](https://wangpengcheng.github.io/2019/05/17/cplusplus_theadpool/)

[HTTP协议](https://zh.wikipedia.org/wiki/%E8%B6%85%E6%96%87%E6%9C%AC%E4%BC%A0%E8%BE%93%E5%8D%8F%E8%AE%AE)
