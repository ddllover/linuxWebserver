#include "webserver.h"

using namespace std;
using std::chrono::system_clock;
void WebServer::Process()
{
    if (isClose_)
        return;
    epoller_->AddFd(servsocket_->getSocket(), listenEvent_ | EPOLLIN);
    LOG_INFO("========== Server start ==========");
    while (!isClose_)
    {
        const auto &eventret = epoller_->Wait(-1);
        for (int i = 0; i < eventret.size(); i++)
        {
            int fd = eventret[i].data.fd;
            uint32_t events = eventret[i].events;
            if (fd == servsocket_->getSocket())
            {
                ClientAccept();
            }
            else
            { // 处理收发
                assert(users_.find(fd) != users_.end());
                auto client = &users_[fd];
                if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
                {
                    ClientClose(&users_[fd]);
                }
                else
                {
                    client->Update();
                    timeque_.push({system_clock::now(), fd});
                    if (events & EPOLLIN)
                    {
                        threadpool_->AddTask(std::bind(&WebServer::ClientRcv, this, client));
                    }
                    else if (events & EPOLLOUT)
                    {
                        threadpool_->AddTask(std::bind(&WebServer::ClientWri, this, client));
                    }
                    else
                        LOG_ERROR("Unexpected event");
                }
            }
        }
        CloseTimeout();
    }
}
void WebServer::CloseTimeout()
{
    auto now = system_clock::now();
    while (!timeque_.empty())
    {
        auto tmp = timeque_.front();
        if (now - tmp.first > timeoutMS_)
        {
            timeque_.pop();
            if (now - users_[tmp.second].timepoint_ > timeoutMS_)
            { // 确实超时 删除
                if (!users_[tmp.second].IsClose())
                {
                    LOG_INFO("client[%d] time out %.0f ms", tmp.second, timeoutMS_.count());
                    ClientClose(&users_[tmp.second]);
                }
            }
        }
        else
            break;
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
        users_[fd].Init(fd, addr);
        timeque_.push({system_clock::now(), fd});
        epoller_->AddFd(fd, EPOLLIN | clientEvent_);
        LOG_INFO("Client[%d](%s:%d) accept, userCount:%d", fd, users_[fd].GetIP(), users_[fd].GetPort(), userCount_.load());
    } while (listenEvent_ & EPOLLET);
}

void WebServer::ClientClose(HttpConn *client)
{
    assert(client);
    if (!client->IsClose())
    {
        epoller_->DelFd(client->GetFd());
        client->Close();
        userCount_--;
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", client->GetFd(), client->GetIP(), client->GetPort(), userCount_.load());
    }
}

void WebServer::ClientRcv(HttpConn *client)
{
    assert(client);
    int ret = -1;
    ret = client->Read();
    if (ret < 0 && errno != EAGAIN) // Et可以异常返回
    {
        LOG_ERROR("client[%d] recv %s", client->GetFd(), strerror(errno));
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
        LOG_ERROR("client[%d] send %s", client->GetFd(), strerror(errno));
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
