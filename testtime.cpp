#include <string_view>
#include <iostream>
#include <thread>
#include <regex>
#include <functional>
#include <future>
#include <ctime>
#include<mutex>
using namespace std;
mutex one;
int t;
void make(){
    
    //one.lock();
    //one.lock();
    t=1;
    //unique_lock<mutex> lk(one);
}
int main(int args,char*argv[])
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
    //测试unique_lock效率
    //unique_lock<mutex> lk(one);
    //unique_lock<mutex> lm(one);
    //make(1);
    one.lock();
    one.lock();
    thread a(make);
    //thread b(make);
    a.join();
    //b.join();
    auto end = time.now();
    std::chrono::duration<double> diff = end - start;
    cout << diff.count() << '\n';
    return 0;
}