#include <unistd.h>
#include "server/webserver.h"
#include "src/log.h"
#include <cstring>
#include <csignal>
#include <netdb.h>
using namespace std;
void signalHandler(int signum)
{
    cout << "Interrupt signal (" << signum << ") received.\n";
    exit(signum);
}
int main(int argc, char *argv[])
{
    signal(SIGINT, signalHandler);
    bool logflag = true;
    int logrank = 1,threadnum = 6,sqlnum = 6,port=80;
    int opt;
    const char *str = "p:s:t:f:l:";
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        ('p' == opt) && (port = atoi(optarg));
        ('s' == opt) && (sqlnum = atoi(optarg));
        ('t' == opt) && (threadnum = atoi(optarg));
        ('f' == opt) && (logflag = atoi(optarg));
        ('l' == opt) && (logrank = atoi(optarg));
    }
    // Mysql 先域名解析

    
    if(logflag) Log::getLog().Init( logrank, 1000000);      // 等级越高过滤越多 
    //域名解析
    auto t=gethostbyname("mysql");                             
    char tmp[INET_ADDRSTRLEN]={0};
    inet_ntop(t->h_addrtype,t->h_addr_list[0],tmp,INET_ADDRSTRLEN);
    LOG_INFO("mysql addr: %s",tmp);
    SqlConnPool::Instance()->Init(tmp, 3306, "", "", "yourdb",sqlnum); // Mysql配置 连接池数量
    WebServer server(threadnum);
    LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", sqlnum, threadnum);
    HttpResponse::srcDir="/root/linuxWebserver/resources/"; //资源路径
    server.InitWebserver("0.0.0.0",to_string(port).c_str(), true, true); // port
    server.Process();
    return 0;
}
