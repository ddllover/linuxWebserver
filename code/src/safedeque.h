#pragma once
#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>

template <class T>
class SafeDeque
{
private:
    std::deque<T> deq_;

    size_t  capacity_;

    std::mutex mtx_;

    bool isClose_;

    std::condition_variable condConsumer_;

    std::condition_variable condProducer_;

public:
    explicit SafeDeque(size_t MaxCapacity = 1000): capacity_(MaxCapacity)
    {   
        isClose_ = false;
    }

    ~SafeDeque()
    {
        Close();
    }

    void clear()
    {
        std::lock_guard<std::mutex> locker(mtx_);
        deq_.clear();
    }

    bool empty()
    {
        std::lock_guard<std::mutex> locker(mtx_);
        return deq_.empty();
    }

    bool full()
    {
        std::lock_guard<std::mutex> locker(mtx_);
        return deq_.size() >=  capacity_;
    }

    void Close()
    {
        {
            std::lock_guard<std::mutex> locker(mtx_);
            deq_.clear();
            isClose_ = true;
        }
        condProducer_.notify_all();
        condConsumer_.notify_all();
    }

    size_t size()
    {
        std::lock_guard<std::mutex> locker(mtx_);
        return deq_.size();
    }

    size_t  capacity()
    {
        std::lock_guard<std::mutex> locker(mtx_);
        return  capacity_;
    }

    T front()
    {
        std::lock_guard<std::mutex> locker(mtx_);
        return deq_.front();
    }

    T back()
    {
        std::lock_guard<std::mutex> locker(mtx_);
        return deq_.back();
    }

    void push_back(const T &item)
    {
        std::unique_lock<std::mutex> locker(mtx_);
        while (deq_.size() >=  capacity_)
        {
            condProducer_.wait(locker);
        }
        deq_.push_back(item);
        condConsumer_.notify_one();
    }

    void push_front(const T &item)
    {
        std::unique_lock<std::mutex> locker(mtx_);
        while (deq_.size() >=  capacity_)
        {
            condProducer_.wait(locker);
        }
        deq_.push_front(item);
        condConsumer_.notify_one();
    }

    bool pop_front(T &item)
    {
        std::unique_lock<std::mutex> locker(mtx_);
        while (deq_.empty())
        {
            condConsumer_.wait(locker);
            if (isClose_)
            {
                return false;
            }
        }
        item = deq_.front();
        deq_.pop_front();
        condProducer_.notify_one();
        return true;
    }

    bool pop_front(T &item, int timeout)
    {
        std::unique_lock<std::mutex> locker(mtx_);
        while (deq_.empty())
        {
            if (condConsumer_.wait_for(locker, std::chrono::seconds(timeout)) == std::cv_status::timeout)
            {
                return false;
            }
            if (isClose_)
            {
                return false;
            }
        }
        item = deq_.front();
        deq_.pop_front();
        condProducer_.notify_one();
        return true;
    }

    void flush()
    {
        condConsumer_.notify_one();
    }
};