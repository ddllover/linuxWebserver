#pragma once
#include <iostream>
#include <unistd.h>
#include <assert.h>
#include <cstring>
template <typename T>
class SimVector    // 1.保证申请的内存是干净的
{                  // 2.capacity_>size 即left不为0，且内存的增长是缓慢的
protected:         // 3.只保证值类型，需要更强大模板请使用vector
                   //  4.不提供对已经申请的做任何操作,
    int capacity_; // 5.目前不打算提供缩小空间的操作
    int size_;
    T *data_ = nullptr;

public:
    SimVector(int capacity = 1024, int marknum = 0) : capacity_(capacity), size_(0)
    {
        if (capacity_ != 0)
        {   //printf("SimVector 构造\n");
            data_ = (T *)calloc(capacity_, sizeof(T));
        }
    }
    SimVector(const SimVector &) = delete; // unique_
    SimVector(SimVector &&) = delete;
    SimVector &operator=(const SimVector &) = delete;
    SimVector &operator=(SimVector &&) = delete;
    T operator[](int i) const
    {
        assert(i >= 0 && i <= size_);
        return *(data_ + i);
    }
    T *begin()
    {
        return data_;
    }
    T *end()
    {
        return data_ + size_;
    }
    void reserve(int size) // 只动capacity_
    {                      // 容器至少比size，并非是size
        if (capacity_ > size)
            return;
        printf("reserve\n");
        T *tmp = (T *)realloc(data_, (size + 2048) * sizeof(T));
        if (tmp)
            data_ = tmp;
        else
        {
            free(data_);
            perror("realloc error ");
        }
        memset(data_ + size_, 0, (size + 2048 - capacity_) * sizeof(T));
        capacity_ = size + 2048; // 每次多预留2048空间
    }
    bool resize(int size) // 只动 size_ 和capacity_
    {
        if (size >= 0)
            size_ = size;
        if (capacity_ > size_)
            return false;
        reserve(size);
        return true;
    }
    int leftsize() const
    {
        return capacity_ - size_;
    }
    int size() const
    {
        return size_;
    }
    int capacity() const
    {
        return capacity_;
    }
    T *data() // 缓冲区的读取都是在外面
    {
        return data_;
    }
    ~SimVector()
    {
        if (data_)
            free(data_);
    }
};

class Buff : public SimVector<char>
{
private:
    int peek_;

public:
    Buff() : SimVector<char>() { peek_ = 0; }
    void PeekAdd(int len) { peek_ += len; }
    const char *Peek() { return data_ + peek_; }
    bool TryEarsePeek(int mark=20480)
    {
        if (capacity_ > mark)
        {
            memcpy(data_, Peek(), size_ - peek_);
            peek_=0;
            size_ = size_ - peek_;
            bzero(end(), capacity_ - size_);
            return true;
        }
        return false;
    }
    int ReadFd(int fd, int *error)
    {
        int len = read(fd, end(), leftsize());
        if (len < 0)
        {
            *error = errno;
        }
        else
        {
            resize(size_ + len);
        }
        return len;
    }
    int WriteFd(int fd, int *saveErrno)
    {
        int len = write(fd, Peek(), size_ - peek_);
        if (len < 0)
        {
            *saveErrno = errno;
            return len;
        }
        else
        {
            peek_ += len;
        }
        return len;
    }
    void clear()
    {
        bzero(data_, capacity_);
        size_ = 0;
        peek_ = 0;
    }
};