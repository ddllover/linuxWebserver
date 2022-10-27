#include <string_view>
#include <iostream>
#include <thread>
#include <regex>
#include <functional>
#include <future>
#include <ctime>
using namespace std;
class T{
    public:
    void add(const char tmp[]){
        cmatch subMatch;
        static regex patten("^([^\r\n]*)\r\n", regex::ECMAScript);
        if (regex_search(tmp, subMatch, patten))
        {   
            cout << subMatch[0] << '\n';
            //cout << tmp + subMatch.length() << "\n";
        }
    }
};
int main()
{
    T a;
    chrono::system_clock t;
    //clock_t clock;
 
    const char tmp[] = "121344\r\n1234\r\n";
    auto start=t.now();
    for (int i = 0; i < 100000; i++)
    {   
        a.add(tmp);
    }
    auto end=t.now();
    std::chrono::duration<double> diff = end-start;
    cout<<diff.count()<<'\n';
    return 0;
}