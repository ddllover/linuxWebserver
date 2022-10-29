/*
 * @Author       : mark
 * @Date         : 2020-06-15
 * @copyleft Apache 2.0
 */

#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>   // readv/writev
#include <arpa/inet.h> // sockaddr_in
#include <stdlib.h>    // atoi()
#include <errno.h>

#include "../src/simvector.h"
#include "../src/log.h"
#include "../pool/sqlconnRAII.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn
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

    HttpRequest request_;
    HttpResponse response_;

public:
    static bool isET;
    static const char *srcDir;
    HttpConn()
    {
        fd_ = -1;
        addr_ = {0};
        isClose_ = true;
    }
    ~HttpConn()
    {
        Close();
    }
    void init(int fd, const sockaddr_in &addr)
    {
        assert(fd > 0);
        addr_ = addr;
        fd_ = fd;
        sendBuff_.clear();
        rcvBuff_.clear();
        sendFile_.data_ = nullptr;
        sendFile_.size_ = 0;
        isClose_ = false;
    }
    void Close()
    {
        response_.UnmapFile();
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
        return request_.IsKeepAlive();
    }
};

#endif // HTTP_CONN_H