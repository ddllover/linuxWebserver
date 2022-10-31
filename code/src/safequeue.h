
#pragma once
#include <mutex>
#include <queue>
template <typename T> // Thread safe implementation of a Queue using a std::queue
class SafeQueue
{
private:
    std::queue<T> queue_; // 利用模板函数构造队列
    std::mutex mutex_; // 访问互斥信号量
public:
    SafeQueue(){}
    SafeQueue(SafeQueue &other){ }
    ~SafeQueue(){ }
    bool empty()
    { 
        std::lock_guard<std::mutex> lock(mutex_); 
        return queue_.empty();
    }
    int size()
    {
        std::lock_guard<std::mutex> lock(mutex_); 
        return queue_.size();
    }
    void push(T &t)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(t);
    }
    T front(){
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.front();
    }
    void pop(){
        std::lock_guard<std::mutex> lock(mutex_); 
        queue_.pop();
    }
    bool dequeue(T &t)
    {
        std::lock_guard<std::mutex> lock(mutex_); 
        if (queue_.empty())
        {
            return false;
        }
        t = std::move(queue_.front()); 
        queue_.pop(); 
        return true;
    }
};