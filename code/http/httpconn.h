#pragma once
#include <sys/types.h>
#include <sys/uio.h>   // readv/writev
#include <arpa/inet.h> // sockaddr_in
#include <stdlib.h>    // atoi()
#include <errno.h>
#include <chrono>

#include "../src/simvector.h"
#include "../src/log.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn:public HttpResponse
{
private:
    int fd_;
    struct sockaddr_in addr_;
    bool isClose_;
    struct file
    {
        char *data_;
        int size_;
    } sendFile_;
    Buff rcvBuff_;  // 读缓冲区
    Buff sendBuff_; // 写缓冲区

    //HttpRequest request_;
    //HttpResponse response_;

public:
    static bool isET;
    std::chrono::system_clock::time_point timepoint_;
    HttpConn():HttpResponse()
    {   
        timepoint_=std::chrono::system_clock::now();
        fd_ = -1;
        addr_ = {0};
        isClose_ = true;
    }
    ~HttpConn()
    {
        Close();
    }
    void Update(){
        timepoint_=std::chrono::system_clock::now();
    }
    void Init(int fd, const sockaddr_in &addr)
    {   
        assert(fd > 0);
        Update();
        //addr_= addr;
        memcpy(&addr_,&addr,sizeof(addr));
        fd_ = fd;
        sendBuff_.clear();
        rcvBuff_.clear();
        sendFile_.data_ = nullptr;
        sendFile_.size_ = 0;
        isClose_ = false;
    }
    void Close()
    {
        UnmapFile();
        if (!isClose_)
        {
            isClose_ = true;
            close(fd_);
        }
    }
    int Read();

    int Write();

    bool ReadAndMake();

    int GetFd() const
    {
        return fd_;
    }
    int GetPort() const
    {
        return addr_.sin_port;
    }
    const char *GetIP() const
    {
        return inet_ntoa(addr_.sin_addr);
    }

    int ToWriteBytes()
    {
        return sendFile_.size_ + (sendBuff_.peekleft());
    }

    bool IsKeepAlive() const
    {
        return isKeepAlive_;
    }
};