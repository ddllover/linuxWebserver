#pragma once
#include <unordered_map>
#include <map>
#include <queue>
#include <filesystem>

#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../sql/sqlconnpool.h"
#include "../http/httpconn.h"
#include "../src/epoll.h"
#include "../src/servsocket.h"
#include "../src/log.h"
#include "../src/threadpool.h"
class WebServer
{
private:
    // 基本设置
    // bool openLinger_;
    std::chrono::duration<double, std::milli> timeoutMS_;
    // int timeoutMS_=5000; /* 毫秒MS */
    bool isClose_ = false;
    uint32_t listenEvent_ = EPOLLRDHUP;
    uint32_t clientEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    // 组件
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoll> epoller_;
    std::unique_ptr<ServSocket> servsocket_;
    // 服务对象
    std::unordered_map<int, HttpConn> users_;
    std::queue<std::pair<std::chrono::system_clock::time_point, int>> timeque_;
    const int MAX_FD = 65536;
    std::atomic<int> userCount_ = 0;
    // 过程
    void ClientAccept();
    // 三个函数要注意保证线程安全只能处理client相关以及Epoll ServSockt
    void ClientClose(HttpConn *client);
    void ClientRcv(HttpConn *client);
    void ClientWri(HttpConn *client);
    void CloseTimeout();

public: // 配置
    WebServer(int threadNum) : timeoutMS_(1000)
    {
        threadpool_ =std::make_unique<ThreadPool>(threadNum);
        epoller_ = std::make_unique<Epoll>();
    }
    ~WebServer() //组件有序关闭
    {   epoller_.reset();
        servsocket_.reset();
        threadpool_.reset();
    }
    void InitWebserver(const char ip[], const char port[], bool clientET = false, bool listenET = false, int timeoutMS = 0)
    {
        servsocket_.reset(new ServSocket(ip, port));
        if (listenET)
            listenEvent_ |= EPOLLET;
        if (clientET)
            clientEvent_ |= EPOLLET;
        HttpConn::isET = (clientEvent_ & EPOLLET);
        if (timeoutMS > 0)
            timeoutMS_ = std::chrono::milliseconds{timeoutMS};
        //HttpResponse::srcDir =std::filesystem::current_path().string()+"/resources/";
        
        // std::string(srcDir_)+"/resources/";
        {
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Ip:%s Port:%s socket:%d", ip, port,servsocket_->getSocket());
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s", listenET ? "ET" : "LT", clientET ? "ET" : "LT");
            LOG_INFO("srcDir: %s",HttpResponse::srcDir.data());
        }
    }
    // 对外接口
    void Process();
};