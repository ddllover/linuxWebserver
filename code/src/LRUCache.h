#include <list>
#include <unordered_map>

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
        Node(KEY a, VALUE b, int c) : key(a), value(b) { size = c; }
    };
    list<Node> list_; // 删尾 提头
    unordered_map<KEY, decltype(list_.begin())> map_;

public:
    LRUCache(int capacity)
    {
        capacity_ = capacity;
        size_ = 0;
    }
    ~LRUCache() {}
    auto find(KEY key) -> decltype(list_.begin())
    {
        auto it = map_.find(key);
        if (it != map_.end())
        { // 指向pos之前,避免删除重新构造
            list_.splice(list_.begin(), list_, it->second);
            map_[key] = list_.begin();
            return list_.begin();
        }
        return list_.end();
    }
    auto end() -> decltype(list_.begin())
    {
        return list_.end();
    }
    void put(KEY key, VALUE value, int size = 1)
    {
        auto it = map_.find(key);
        if (it != map_.end()) // 更新
        {
            it->second->value = value;
            size_ = size_ - it->second->size + size;
            it->second->size = size;

            list_.splice(list_.begin(), list_, it->second);
        }
        else
        {
            size_ += size; // 先更新大小
            // 先判断是否满，满了要删除
            while (size_ >= capacity_)
            {
                // int del_key = ;
                map_.erase(list_.back().key);
                size_ -= list_.back().size;
                list_.pop_back();
            }
            // 插入到 map_, list_ 中
            list_.push_front(Node(key, value, size));
            map_[key] = list_.begin();
        }
    }
};
