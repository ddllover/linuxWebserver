/*
 * @Author       : mark
 * @Date         : 2020-06-17
 * @copyleft Apache 2.0
 */

#include "webserver.h"

using namespace std;

WebServer::WebServer(
    int port, int trigMode, int timeoutMS, bool OptLinger,
    int sqlPort, const char *sqlUser, const char *sqlPwd,
    const char *dbName, int connPoolNum, int threadNum,
    bool openLog, int logLevel, int logQueSize)
    : port_(port), openLinger_(OptLinger), timeoutMS_(timeoutMS), isClose_(false),
      timer_(new HeapTimer()), threadpool_(new ThreadPool(threadNum)), epoller_(new Epoll()),
      servsocket_(new ServSocket("127.0.0.1", to_string(port)))
{
    srcDir_ = getcwd(nullptr, 256);
    assert(srcDir_);
    strncat(srcDir_, "/resources/", 16);
    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir_;
    SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);

    InitEventMode_(trigMode);

    if (openLog)
    {
        Log::Instance()->init(logLevel, "./log", ".log", logQueSize);
        if (isClose_)
        {
            LOG_ERROR("========== Server init error!==========");
        }
        else
        {
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port:%d, OpenLinger: %s", port_, OptLinger ? "true" : "false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                     (listenEvent_ & EPOLLET ? "ET" : "LT"),
                     (connEvent_ & EPOLLET ? "ET" : "LT"));
            LOG_INFO("LogSys level: %d", logLevel);
            LOG_INFO("srcDir: %s", HttpConn::srcDir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connPoolNum, threadNum);
        }
    }
}

WebServer::~WebServer()
{
    close(servsocket_->getSocket());
    isClose_ = true;
    free(srcDir_);
    SqlConnPool::Instance()->ClosePool();
}

void WebServer::InitEventMode_(int trigMode)
{
    listenEvent_ = EPOLLRDHUP;
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        connEvent_ |= EPOLLET;
        break;
    case 2:
        listenEvent_ |= EPOLLET;
        break;
    case 3:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    default:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}

void WebServer::Start()
{
    epoller_->AddFd(servsocket_->getSocket(), listenEvent_ | EPOLLIN);
    int timeMS = -1; /* epoll wait timeout == -1 无事件将阻塞 */
    if (!isClose_)
    {
        LOG_INFO("========== Server start ==========");
    }
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
            else if (events & EPOLLIN)
            {
                assert(users_.count(fd) > 0);
                ClientRcv(&users_[fd]);
            }
            else if (events & EPOLLOUT)
            {
                assert(users_.count(fd) > 0);
                ClientWri(&users_[fd]);
            }
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                assert(users_.count(fd) > 0);
                ClientClose(&users_[fd]);
            }
            else
            {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

void WebServer::SendError_(int fd, const char *info)
{
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret < 0)
    {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

void WebServer::ClientAccept()
{
    do
    {
        struct sockaddr_in addr = {0};
        int fd = servsocket_->Accept(&addr);
        if (fd <= 0)
        {
            return;
        }
        else if (HttpConn::userCount >= MAX_FD)
        {
            perror("server bad\n");
            SendError_(fd, "Server busy!");
            LOG_WARN("Clients is full!");
            return;
        }
        users_[fd].init(fd, addr);
        if (timeoutMS_ > 0)
        {
            timer_->add(fd, timeoutMS_, std::bind(&WebServer::ClientClose, this, &users_[fd]));
        }
        epoller_->AddFd(fd, EPOLLIN | connEvent_);
    } while (listenEvent_ & EPOLLET);
}

void WebServer::ClientRcv(HttpConn *client)
{
    assert(client);
    if (timeoutMS_ > 0)
    {
        timer_->adjust(client->GetFd(), timeoutMS_);
    }
    threadpool_->AddTask(std::bind(&WebServer::OnRead_, this, client));
}

void WebServer::ClientWri(HttpConn *client)
{
    assert(client);
    if (timeoutMS_ > 0)
    {
        timer_->adjust(client->GetFd(), timeoutMS_);
    }
    threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, client));
}

void WebServer::ClientClose(HttpConn *client)
{
    assert(client);
    LOG_INFO("Client[%d] quit!", client->GetFd());
    epoller_->DelFd(client->GetFd());
    client->Close();
}
void WebServer::OnRead_(HttpConn *client)
{
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    if (ret <= 0 && readErrno != EAGAIN)
    {
        ClientClose(client);
        return;
    }
    if (client->ReadAndMake())
    {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
    }
    else
    {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
    }
}

void WebServer::OnWrite_(HttpConn *client)
{
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if (client->ToWriteBytes() == 0)
    {
        if (client->IsKeepAlive())
        {
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
            return ;
        }
    }
    else if (ret < 0)
    {
        if (writeErrno == EAGAIN)//ET模式必须需要的选项
        {
            /* 继续传输 */
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    ClientClose(client);
}
