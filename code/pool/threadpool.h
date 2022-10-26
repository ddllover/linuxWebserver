#pragma once
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <functional>
#include <future>
#include <utility>
using namespace std;
/*class ThreadWork
{
public:
    vector<int> clientSocket; //每个线程自己服务几个客户端
    mutex threadMutex;        //文件描述符 互斥符
    condition_variable mutexConVar;
    Epoll epoll;
    HttpServe httpserve;
    ThreadWork() : epoll(), clientSocket() {}
    void operator()()
    {
        while (true)
        {
            {
                unique_lock<mutex> lk(threadMutex);
                if (clientSocket.empty() && epoll.fdnum() > 0)
                {
                    mutexConVar.wait(lk, [this]
                                     { return clientSocket.empty(); });
                }
                for (auto &i : clientSocket)
                    epoll.AddFd(i);
                clientSocket.clear();
                mutexConVar.notify_one(); //只有主线程会竞争这个锁
            }
            const std::vector<struct epoll_event> &ans = epoll.Wait();
            for (auto &i : ans)
                httpserve.Process(i.data.fd);
        }
    }
};*/

class ThreadPool
{
private:
    class ThreadWorker
    { //内置线程工作类

    private:
        int id;           //工作id
        ThreadPool *pool; //所属线程池
    public:
        ThreadWorker(ThreadPool *pool, const int id) : pool(pool), id(id) {}
        void operator()()
        {
            std::function<void()> func; //定义基础函数类func
            //判断线程池是否关闭，没有关闭，循环提取
            unique_lock<mutex> lk(pool->mutex_);
            while (!pool->isClose)
            {
                if (!pool->taskDeque.empty())
                {
                    func = pool->taskDeque.front();
                    pool->taskDeque.pop_front();
                    lk.unlock();
                    func();
                    lk.lock();
                }
                else
                {
                    pool->conMutex_.wait(lk);
                }
            }
        }
    };

    bool isClose = false;              //线程池是否关闭
    deque<function<void()>> taskDeque; //执行函数安全队列，即任务队列
    vector<thread> threads;            //工作线程队列
    mutex mutex_;
    condition_variable conMutex_;

public:
    //线程池构造函数
    explicit ThreadPool(const int n_threads) : threads(vector<thread>(n_threads))//, taskDeque(1000000)
    {
        for (int i = 0; i < (int)threads.size(); ++i)
        {
            threads[i] = thread(ThreadWorker(this, i)); //分配工作线程
        }
    }
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;
    ~ThreadPool() { shutdown(); }
    void shutdown()
    { // 强制操作控制线程
        isClose = true;
        // taskDeque.Close();
        for (int i = 0; i < (int)threads.size(); ++i)
        {
            if (threads[i].joinable())
            {
                threads[i].join(); //将线程加入等待队列
            }
        }
    }
    template <typename F, typename... Args>
    auto AddTask(F &&f, Args &&...args) -> std::future<decltype(f(args...))>
    { // bind绑定部分参数 function 只可以绑定可复制的对象  bind会范围更广但 只有全是可复制的才能复制
        function<decltype(f(args...))()> func = bind(forward<F>(f), forward<Args>(args)...);
        //封装获取任务对象，方便另外一个线程查看结果
        auto task_ptr = make_shared<packaged_task<decltype(f(args...))()>>(func);
        // Wrap packaged task into void function
        function<void()> wrapper_func = [task_ptr]()
        {
            (*task_ptr)();
        };
        // 队列通用安全封包函数，并压入安全队列
        {
            lock_guard<mutex> lk(mutex_);
            taskDeque.push_back(wrapper_func);
        }
        conMutex_.notify_one();
        // 返回先前注册的任务指针
        return task_ptr->get_future();
    }
};
