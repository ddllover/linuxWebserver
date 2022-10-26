#pragma once
#include <iostream>
#include <cstring>
#include <assert.h>
#ifdef __linux__
#include <sys/socket.h> 
#include <unistd.h> 
#include <arpa/inet.h>
#include <fcntl.h>
#elif  _WIN32
#include <winsock2.h>
#pragma comment (lib, "ws2_32.lib")
#endif

class ServSocket
{
private:
    int servSocket;
    //int openLinger_=1;
    struct sockaddr_in servIpv4; //专用于ipv4的地址方便查询 sockaddr通用的
public:
    ServSocket()=delete;
    ServSocket(std::string ipv4, std::string port, int _domain = AF_INET, int _type = SOCK_STREAM|SOCK_NONBLOCK, int _prpotocol = IPPROTO_TCP){
        /*地址族ipv4*/ /*套接字类型，面向连接*/ /*传输协议 TCP*/
        servSocket = socket(_domain, _type, _prpotocol);
        fcntl(servSocket, F_SETFL, fcntl(servSocket, F_GETFL, 0) | O_NONBLOCK);
        assert(servSocket>=0);
        servIpv4.sin_family = _domain;                          //使用IPv4地址
        servIpv4.sin_addr.s_addr = inet_addr(ipv4.c_str());     //具体的IP地址
        servIpv4.sin_port = htons(std::stoi(port));             //端口
        memset(servIpv4.sin_zero, 0, sizeof(servIpv4.sin_zero)); //只是为了补全 用于转化为sockaddr_in 转化为sockaddr
        
    }
    void Bind(){
        //struct linger optLinger = { 0 };
        //if(openLinger_) {
            /* 优雅关闭: 直到所剩数据发送完毕或超时 */
        //    optLinger.l_onoff = 1;
        //   optLinger.l_linger = 1;
        //}
        //setsockopt(servSocket, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
        int optval = 1;
        setsockopt(servSocket, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
        assert(bind(servSocket,reinterpret_cast<struct sockaddr *>(&servIpv4),sizeof(servIpv4))>=-1);

    }
    void Listen(int size=SOMAXCONN){
        if(listen(servSocket,SOMAXCONN)<0) perror("listen\n");
    }
    int Accept(struct sockaddr_in * clientIpv4=nullptr){
        #ifdef __linux__
        socklen_t clientIpv4Len=sizeof(sockaddr_in::sin_addr);
        #elif _WIN32
        int clientIpv4Len=sizeof(clientIpv4->sin_addr);
        #endif
        int clientSocket=accept(servSocket,reinterpret_cast<struct sockaddr *>(clientIpv4),&clientIpv4Len);
        if(clientSocket<0) {
            perror("client accept\n");
        }
        fcntl(clientSocket, F_SETFL, fcntl(clientSocket, F_GETFL, 0) | O_NONBLOCK);
        return clientSocket;
    }
    int getSocket() const {return servSocket;}
    /*
    int Read(int clientSocket,char buf[]){
        return read(clientSocket,buf,sizeof(buf));
    }
    int Write(int clientSocket,const char buf[]){
        return write(clientSocket,buf,sizeof(buf));
    }
    int Close(int clientSocket){
        return close(clientSocket);
    }*/
    ~ServSocket(){
        #ifdef __linux__
        assert(close(servSocket)>=0);
        #elif  _WIN32
        assert(closesocket(servSocket)>=0);
        #endif
    }
};