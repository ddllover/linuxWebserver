#include <unistd.h>
#include "server/webserver.h"
#include "src/log.h"
#include <cstring>

int main(int args, char *argv[])
{
    /* 守护进程 后台运行 */
    // daemon(1, 0);
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
    WebServer server(6); //线程数
    server.InitWebserver("127.0.0.1", "1234",true,true);//port
    server.Process();
    SqlConnPool::Instance()->ClosePool();
}
