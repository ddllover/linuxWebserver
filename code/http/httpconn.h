/*
 * @Author       : mark
 * @Date         : 2020-06-15
 * @copyleft Apache 2.0
 */ 

#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      

#include "../src/simvector.h"
#include "../src/log.h"
#include "../pool/sqlconnRAII.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
public:
    HttpConn();

    ~HttpConn();

    void init(int sockFd, const sockaddr_in& addr);

    ssize_t Read(int* saveErrno);

    ssize_t Write(int* saveErrno);

    void Close();

    int GetFd() const;

    int GetPort() const;

    const char* GetIP() const;
    
    sockaddr_in GetAddr() const;
    
    bool ReadAndMake();

    int ToWriteBytes() { 
        return sendFile_.size_+(sendBuff_.peekleft()); 
    }

    bool IsKeepAlive() const {
        return request_.IsKeepAlive();
    }

    static bool isET;
    static const char* srcDir;
    static std::atomic<int> userCount;
    
private:
   
    int fd_;
    struct  sockaddr_in addr_;

    bool isClose_;

    struct file
    {
        char * data_;
        int size_;
    }sendFile_;
    Buff rcvBuff_; // 读缓冲区
    Buff sendBuff_; // 写缓冲区


    HttpRequest request_;
    HttpResponse response_;
};


#endif //HTTP_CONN_H