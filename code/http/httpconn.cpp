
#include "httpconn.h"
using namespace std;

const char *HttpConn::srcDir;
bool HttpConn::isET;

HttpConn::HttpConn()
{
    fd_ = -1;
    addr_ = {0};
    isClose_ = true;
};

HttpConn::~HttpConn()
{
    Close();
};

void HttpConn::init(int fd, const sockaddr_in &addr)
{
    assert(fd > 0);
    addr_ = addr;
    fd_ = fd;
    sendBuff_.clear();
    rcvBuff_.clear();
    sendFile_.data_=nullptr;
    sendFile_.size_=0;
    isClose_ = false;
    
}

void HttpConn::Close()
{
    response_.UnmapFile();
    if (!isClose_)
    {
        isClose_ = true;
        close(fd_);
    }
}

int HttpConn::GetFd() const
{
    return fd_;
};

struct sockaddr_in HttpConn::GetAddr() const
{
    return addr_;
}

const char *HttpConn::GetIP() const
{
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::GetPort() const
{
    return addr_.sin_port;
}

ssize_t HttpConn::Read(int *saveErrno)
{
    ssize_t len = -1;
    do
    {
        len = rcvBuff_.ReadFd(fd_, saveErrno);
        if(len<0) break; //ET 模式下必须读取到错误才能返回
    } while (isET);
    return len;
}

ssize_t HttpConn::Write(int *saveErrno)
{
    ssize_t len = -1;
    do
    {
        if (sendBuff_.peekleft()!=0)
        {
            len = sendBuff_.WriteFd(fd_, saveErrno);
            if (len <0)
                break;
            //printf("%d len sendbuff\n",len);
        }
        else
        { // send file
            if (sendFile_.size_ > 0)
            {
                len = write(fd_, sendFile_.data_, sendFile_.size_);
                if (len <0)
                {
                    *saveErrno = errno;
                    break;
                }
                sendFile_.data_ += len;
                sendFile_.size_ -= len;
                //printf("%d sendfilelen %d sendFile_.size_\n",len,sendFile_.size_);
            }
        }
        if(ToWriteBytes()==0){  // 写自己可以判断是否写完，写完即可退出
            //printf("%s",sendBuff_.data());
            sendBuff_.clear(); 
            break;
        }
    } while (isET || ToWriteBytes() > 10240);
    return len;
}

bool HttpConn::ReadAndMake()
{
    if (false == request_.parse(rcvBuff_))
        return false;

    LOG_DEBUG("%s", request_.path().c_str());

    response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    response_.MakeResponse(sendBuff_);
    if (response_.File() && response_.FileLen() > 0)
    {
        sendFile_.data_ = response_.File();
        sendFile_.size_ = response_.FileLen();
    }

    LOG_DEBUG("filesize:%d  to %d", sendFile_.size_, ToWriteBytes());

    request_.Init();
    rcvBuff_.TryEarsePeek();
    return true;
}
