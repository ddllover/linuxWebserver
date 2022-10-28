#pragma once
#include <iostream>
#include <assert.h>
#include <cstring>
template <typename T>
class SimVector             //1.保证申请的内存是干净的
{                           //2.capacity_>size 即left不为0，
private:                    //3.可以利用mark建立标记
    int *mark_=nullptr;     //4.不提供对已经申请的做任何操作
    int capacity_;          //5.目前不打算提供缩小空间的操作
    int size_;              //6.对象是完全独立的的，只能使用指针或者引用
    T *data_=nullptr;     
public:
    SimVector(int capacity = 0,int marknum=0) : capacity_(capacity), size_(0)
    {   
        if(marknum!=0){
            mark_=new int[marknum];
            memset(mark_,0,marknum*sizeof(int));
        }
        if (capacity_ != 0){
            printf("SimVector 构造\n");
            data_ = (T *)calloc(capacity_ , sizeof(T));
        }
    }
    SimVector(const SimVector&)=delete;  //unique_
    SimVector(SimVector&&)=delete;
    SimVector &operator=(const SimVector&)=delete;
    SimVector &operator=(SimVector&&)=delete;
    int & Mark(int i){
        return mark_[i];
    }
    T operator[](int i)const
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
    /*void erase(int begin, int end) 
    {
        assert(begin <= end && begin >= 0 && end<=size_);
        if(begin!=end)
        memcpy(data_+begin, data_ + end, (size_ - end) * sizeof(T));
        size_=size_-(end-begin);
        //memset(data_+size_,0,(capacity_-size_)*sizeof(T));
    }*/
    /*void append(T *begin, int len)
    {
        reserve(size_ + len);
        memcpy(data_ + size_, begin, len * sizeof(T));
        size_ = size_ + len;
    }*/     
    void reserve(int size)  //只动capacity_
    { //容器至少比size，并非是size
        if (capacity_ > size)
            return;
        printf("reserve\n");
        T *tmp = (T *)realloc(data_, (size + 2048) * sizeof(T));
        if(tmp) data_=tmp;
        else {free(data_);perror("realloc error ");}
        memset(data_+size_,0,(size + 2048-capacity_)*sizeof(T));
        capacity_ = size + 2048; //每次多预留2048空间
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
    ~SimVector() {if(data_) free(data_);if(mark_) delete [] mark_;}
};
