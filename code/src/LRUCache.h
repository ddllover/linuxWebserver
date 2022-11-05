#include <list>
#include <unordered_map>
#include <mutex>
#include <assert.h>
using namespace std;

template <typename KEY, typename VALUE>
class LRUCache
{
private:
    int capacity_; // 100*1024
    int size_;
    struct Node
    {
        KEY key;
        VALUE value;
        int size;
        int cnt = 1; // 保证线程安全,存在线程使用的资源不是释放
        Node() {}
        Node(KEY a, VALUE b, int c) : key(a), value(b)
        {
            size = c;
        }
        Node *next = nullptr;
        Node *last = nullptr;
    };
    Node Head_;
    Node Tail_;
    // list<Node> list_; // 删尾 提头
    unordered_map<KEY, Node *> map_;
    std::mutex mutex_;

public:
    LRUCache(int capacity = 10000000) // 10M
    {
        Head_.next = &Tail_;
        Tail_.last = &Head_;
        capacity_ = capacity;
        size_ = 0;
    }
    ~LRUCache() {}
    bool find(KEY key, VALUE *result = nullptr)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = map_.find(key);
        if (it != map_.end())
        { // 指向pos之前,避免删除重新构造
            if (result)
                it->second->cnt++;
            else
                it->second->cnt--;
            // 断开
            it->second->next->last = it->second->last;
            it->second->last->next = it->second->next;
            // 连接
            it->second->next = Head_.next;
            it->second->last = &Head_;
            Head_.next->last = it->second;
            Head_.next = it->second;
            if(result)
                *result = it->second->value;
            return true;
        }
        return false;
    }
    void put(KEY key, VALUE value, int size = 1)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = map_.find(key);
        if (it != map_.end()) // 正常不更新
        {
            assert(0);
        }
        else
        {
            // 先判断是否满，满了从尾部开始删除//虚拟头尾节点不会删除
            auto it = Tail_.last;
            while (size_ + size > capacity_)
            {
                // int del_key = ;
                if (it == &Head_)
                {
                    break;
                }
                if (it->cnt >= 1)
                {
                    it = it->last;
                }
                else
                {
                    map_.erase(it->key);
                    size_ -= it->size;
                    it->next->last = it->last;
                    it->last->next = it->next;
                    auto tmp = it->last;
                    delete it;
                    it = tmp;
                }
            }
            // 插入到 map_, list_ 中
            if (size_ + size <= capacity_)
            {
                it=new Node(key, value, size);
                it->next = Head_.next;
                it->last = &Head_;
                Head_.next->last = it;
                Head_.next = it;
                map_[key] = it;
                size_ += size;
            }
        }
    }
};
