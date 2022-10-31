#include <string_view>
#include <iostream>
#include <thread>
#include <regex>
#include <functional>
#include <future>
#include <ctime>
#include <mutex>
#include <map>
#include <queue>
#include <assert.h>
#include <mysql/mysql.h>
#include <semaphore>
#include <filesystem>
#include "../code/src/LRUCache.h"
using namespace std;
queue<MYSQL *> connQue_;
void Init(const char *host, int port, const char *user, const char *pwd, const char *dbName, int connSize = 10)
{
    assert(connSize > 0);
    for (int i = 0; i < connSize; i++)
    {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if (!sql)
        {
            //cout<<"erro"<<endl;
            assert(sql);
        }
        sql = mysql_real_connect(sql, host, user, pwd,
                                 dbName, port, nullptr, 0);
        if (!sql)
        {
            cout<<"erro"<<endl;
        }
        connQue_.push(sql);
    }
    //MAX_CONN_ = connSize;
}
int main(int args, char *argv[])
{   
    chrono::system_clock time;
    auto start = time.now();
    /*
    //正则表达式
    regex patten("^([^\r\n]*)\r\n", regex::ECMAScript);
    cmatch subMatch;
    const char tmp[] = "121344\r\n";
    */
    /*
    //智能指针
    int* ptr = new int;
    std::shared_ptr<int> p1(ptr);
    *p1;
    p1.reset();
    if(!p1) cout<<0<<'\n';
    std::weak_ptr<int> p2=p1;
    */
    // string_view
    /*string m="1235";
    string_view y(m.data(),m.size()-1);
    cout<<y<<'\n';*/
    // 测试unique_lock效率
    // unique_lock<mutex> lk(one);
    // unique_lock<mutex> lm(one);
    // make(1);
    // thread b(make);
    // b.join();
    //mysql
    //Init("localhost", 3306, "root", "", "yourdb"); 
    LRUCache<int,int> t(8);
    t.put(1,1,2);
    t.put(2,2,2);
    t.put(4,5,2);
    t.put(6,6,2);
    t.put(4,4,3);
    auto it=t.find(4);
    while(it!=t.end()){
        cout<<it->key<<it->value<<it->size<<endl;
        it++;
    }
    auto end = time.now();
    // std::chrono::duration_values difft();
    std::chrono::duration<double> diff = end - start;
    cout << diff.count() << '\n';
    return 0;
}