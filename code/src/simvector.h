#pragma once
#include <iostream>
#include <assert.h>
#include <cstring>
// 由于是作为存储空间使用，不需要构造函数
template <typename T>
class SimVector //一个保证capacity_>size 即left不为0 且不使用构造函数的容器
{
private:
    int capacity_;
    int size_;
    T *data_=nullptr;

public:
    SimVector(int capacity = 0) : capacity_(capacity), size_(0)
    {
        if (capacity_ != 0){
            printf("SimVector 构造\n");
            data_ = (T *)calloc(capacity_ , sizeof(T));
        }
    }
    T &operator[](int i)
    {
        assert(i >= 0 && i <= size_);
        return *(data_+i);
    }
    T *begin()
    {
        return data_;
    }
    T *end()
    {
        return data_ + size_;
    }
    void erase(int begin, int end) 
    {
        assert(begin <= end && begin >= 0 && end<=size_);
        if(begin!=end)
        memcpy(data_+begin, data_ + end, (size_ - end) * sizeof(T));
        size_=size_-(end-begin);
        memset(data_+size_,0,(capacity_-size_)*sizeof(T));
    }
    void append(T *begin, int len)
    {
        reserve(size_ + len);
        memcpy(data_ + size_, begin, len * sizeof(T));
        size_ = size_ + len;
    }
    void reserve(int size)  //只动capacity_
    { //容器至少比size，并非是size
        if (capacity_ > size)
            return;
        printf("reserve\n");
        T *tmp = (T *)realloc(data_, (size + 1024) * sizeof(T));
        if(tmp) data_=tmp;
        else free(data_);
        memset(data_+size_,0,(size + 1024-capacity_)*sizeof(T));
        capacity_ = size + 1024; //每次多预留2048空间
    }
    void resize(int size)  //只动 size_ 和capacity_
    {   
        if(size>=0)
        size_ = size;
        if (capacity_ > size_)
            return;
        reserve(size);
       
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
    T *data() //缓冲区的读取都是在外面
    {
        return data_;
    }
    ~SimVector() { /*free(data_); */}
};
