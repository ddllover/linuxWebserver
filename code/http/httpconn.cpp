
#include "httpconn.h"
using namespace std;

const char *HttpConn::srcDir;
bool HttpConn::isET;

int HttpConn::Read()
{
    int len = -1;
    do
    {
        len = rcvBuff_.ReadFd(fd_);
        if(len<0) break; //ET 模式下必须读取到错误才能返回
    } while (isET);
    return len;
}

int HttpConn::Write()
{
    int len = -1;
    do
    {
        if (sendBuff_.peekleft()!=0)
        {
            len = sendBuff_.WriteFd(fd_);
            if (len <0)
                break;
            LOG_DEBUG("%d len sendbuff\n",len);
        }
        else
        { // send file
            if (sendFile_.size_ > 0)
            {
                len = write(fd_, sendFile_.data_, sendFile_.size_);
                if (len <0)
                {
                    break;
                }
                sendFile_.data_ += len;
                sendFile_.size_ -= len;
                LOG_DEBUG("%d sendfilelen %d sendFile_.size_\n",len,sendFile_.size_);
            }
        }
        if(ToWriteBytes()==0){ 
            LOG_DEBUG("%s",sendBuff_.data());
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
