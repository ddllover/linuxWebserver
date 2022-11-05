#include "httpconn.h"
using namespace std;
bool HttpConn::isET;

int HttpConn::Read()
{
    int len = -1;
    do
    {
        len = rcvBuff_.ReadFd(fd_);
        if (len < 0)
            break; // ET 模式下必须读取到错误才能返回
    } while (isET);
    return len;
}

int HttpConn::Write()
{
    int len = -1;
    do
    {
        if (sendBuff_.peekleft() != 0)
        {
            len = sendBuff_.WriteFd(fd_);
            if (len < 0)
            {
                LOG_DEBUG("%d len sendbuff", len);
                break;
            }
        }
        else
        { // send file
            if (sendFile_.size_ > 0)
            {
                len = write(fd_, sendFile_.data_, sendFile_.size_);
                if (len < 0)
                {
                    LOG_DEBUG("%d sendfilelen %d sendFile_.size_", len, sendFile_.size_);
                    break;
                }
                sendFile_.data_ += len;
                sendFile_.size_ -= len;
            }
        }
        if (ToWriteBytes() == 0)
        {
            // LOG_DEBUG("sendfile %s", sendBuff_.data());
            sendBuff_.clear();
            sendBuff_.TryEarsePeek();
            ResponseClear();// 发完立马清除
            break;
        }
    } while (isET || ToWriteBytes() > 10240);
    return len;
}

bool HttpConn::ReadAndMake()
{
    if (false == ParseRequest(rcvBuff_))
        return false;
    MakeResponse(sendBuff_);
    if (mmFile_ && mmFileStat_.st_size > 0)
    {
        sendFile_.data_ = mmFile_;
        sendFile_.size_ = mmFileStat_.st_size;
    }
    RequestClear();
    rcvBuff_.TryEarsePeek();
    return true;
}
