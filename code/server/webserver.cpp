/*
 * @Author       : mark
 * @Date         : 2020-06-17
 * @copyleft Apache 2.0
 */

#include "webserver.h"

using namespace std;

void WebServer::Process()
{
    if (isClose_)
        return;
    epoller_->AddFd(servsocket_->getSocket(), listenEvent_ | EPOLLIN);
    LOG_INFO("========== Server start ==========");
    int timeMS = -1;
    while (!isClose_)
    {
        if (timeoutMS_ > 0)
        {
            timeMS = timer_->GetNextTick();
        }
        const auto &eventret = epoller_->Wait(timeMS);
        for (int i = 0; i < eventret.size(); i++)
        {
            int fd = eventret[i].data.fd;
            uint32_t events = eventret[i].events;
            if (fd == servsocket_->getSocket())
            {
                ClientAccept();
            }
            else
            {
                assert(users_.find(fd) != users_.end());
                auto client = &users_[fd];
                if (events & EPOLLIN)
                {
                    if (timeoutMS_ > 0)
                        timer_->adjust(client->GetFd(), timeoutMS_);
                    threadpool_->AddTask(std::bind(&WebServer::ClientRcv, this, client));
                }
                else if (events & EPOLLOUT)
                {
                    if (timeoutMS_ > 0)
                        timer_->adjust(client->GetFd(), timeoutMS_);
                    threadpool_->AddTask(std::bind(&WebServer::ClientWri, this, client));
                }
                else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
                {
                    ClientClose(&users_[fd]);
                }
                else
                    LOG_ERROR("Unexpected event");
            }
        }
    }
}

void WebServer::ClientAccept()
{
    struct sockaddr_in addr = {0};
    do
    {
        int fd = servsocket_->Accept(&addr);
        if (fd <= 0)
        {
            return;
        }
        else if (userCount_ >= MAX_FD)
        {
            close(fd);
            LOG_WARN("Clients is full!");
            return;
        }
        userCount_++;
        users_[fd].init(fd, addr);
        LOG_INFO("Client[%d](%s:%d) accept, userCount:%d", fd, users_[fd].GetIP(), users_[fd].GetPort(), userCount_);
        if (timeoutMS_ > 0)
        {
            timer_->add(fd, timeoutMS_, std::bind(&WebServer::ClientClose, this, &users_[fd]));
        }
        epoller_->AddFd(fd, EPOLLIN | clientEvent_);
    } while (listenEvent_ & EPOLLET);
}

void WebServer::ClientClose(HttpConn *client)
{
    assert(client);
    epoller_->DelFd(client->GetFd());
    client->Close();
    userCount_--;
    LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", client->GetFd(), client->GetIP(), client->GetPort(), userCount_);
}

void WebServer::ClientRcv(HttpConn *client)
{
    assert(client);
    int ret = -1;
    ret = client->Read();
    if (ret < 0 && errno != EAGAIN) // Et可以异常返回
    {
        LOG_ERROR("recv client[%d] %s", client->GetFd(), strerror(errno));
        ClientClose(client);
        return;
    }
    if (client->ReadAndMake())
    {
        epoller_->ModFd(client->GetFd(), clientEvent_ | EPOLLOUT);
    }
    else
    {
        epoller_->ModFd(client->GetFd(), clientEvent_ | EPOLLIN);
    }
}

void WebServer::ClientWri(HttpConn *client)
{
    assert(client);
    int ret = -1;
    ret = client->Write();
    if (ret < 0 && errno != EAGAIN)
    { // Et可以异常返回
        LOG_ERROR("send client[%d] %s", client->GetFd(), strerror(errno));
        ClientClose(client);
        return;
    }
    if (client->ToWriteBytes() == 0)
    {
        if (client->IsKeepAlive())
        {
            epoller_->ModFd(client->GetFd(), clientEvent_ | EPOLLIN);
            return;
        }
        else
            ClientClose(client);
    }
    else
    {
        epoller_->ModFd(client->GetFd(), clientEvent_ | EPOLLOUT);
    }
}
