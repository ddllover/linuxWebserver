#include <unistd.h>
#include "server/webserver.h"
#include "src/log.h"
#include <cstring>
#include <csignal>
WebServer * server; //线程数
void signalHandler( int signum ){
    Log::getLog().shutdown();
    cout << "Interrupt signal (" << signum << ") received.\n";  
    delete server;
    exit(signum);
}
int main(int args, char *argv[])
{
    //signal(SIGINT, signalHandler); 
    int i = 1024;
    bool flag = true;
    if (args >= 2)
    {
        if (strcmp(argv[1], "-t") == 0)
        {
            i = stoi(string(argv[2]));
            flag = true;
        }
    }
    // Mysql 功能
    SqlConnPool::Instance()->Init("localhost", 3306, "root", "", "yourdb", 6); // Mysql配置 连接池数量
    if (flag) Log::getLog().Init(1,1000000); //等级越高过滤越多
    server=new WebServer(6);
    server->InitWebserver("127.0.0.1", "1234",true,true);//port
    server->Process();
    return 0;
}
