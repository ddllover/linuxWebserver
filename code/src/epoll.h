#pragma once
#ifdef __linux__
#include <sys/epoll.h> //epoll_ctl()
#else
#endif
#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <assert.h> // close()
#include <errno.h>
#include <vector>
#include "simvector.h"
class Epoll
{
private:
    int epfd_;
    SimVector<struct epoll_event> events_; // 注意线程不安全的
public:
    Epoll(int maxEvent = 1024, int epollnum = 512) : events_(maxEvent)
    {
        if ((epfd_ = epoll_create(epollnum))<0)
        {
            perror("epoll creat");
        }
        fcntl(epfd_, F_SETFL, fcntl(epfd_, F_GETFD, 0) | O_NONBLOCK);
    }
    bool AddFd(int fd, uint32_t events)
    { // 增加的一个事件作为等读取对面的请求
        if (fd < 0)
            return false;
        epoll_event ev = {0};
        ev.data.fd = fd;
        ev.events = events;
        if (epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) < 0)
        {
            perror("eoor");
            return false;
        }
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
        return true;
    }

    bool ModFd(int fd, uint32_t events)
    {
        if (fd < 0)
            return false;
        epoll_event ev = {0};
        ev.data.fd = fd;
        ev.events = events;
        if (epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev) < 0)
        {
            perror("epoll mod");
            return false;
        }
        return true;
    }

    bool DelFd(int fd)
    {
        if (fd < 0)
            return false;
        epoll_event ev = {0};
        // ev.data.fd=fd;
        if (epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, &ev) < 0)
        {
            // perror("delfd error");
            return false;
        }
        return true;
    }

    const SimVector<struct epoll_event> &Wait(int timeoutMs = -1)
    {
        // events.resize(events.capacity());
        int cnt;
        if ((cnt = epoll_wait(epfd_, events_.data(), events_.capacity(), timeoutMs)) < 0)
        {
            perror("epoll wait");
            events_.resize(0);
        }
        else
            events_.resize(cnt);
        return events_;
    }
    ~Epoll()
    {
        close(epfd_); // vector 会自己析构 除非存指针有申请堆内存
    }
};