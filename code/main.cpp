/*
 * @Author       : mark
 * @Date         : 2020-06-18
 * @copyleft Apache 2.0
 */ 
#include <unistd.h>
#include "server/webserver.h"
#include <cstring>

int main(int args, char *argv[]) {
    /* 守护进程 后台运行 */
    //daemon(1, 0); 
    int i=1024;
    bool flag=false;
    if(args>=2){
        if(strcmp(argv[1],"-t")==0){
            i=stoi(string(argv[2]));
            flag=true;
        }
   
    }
    
    WebServer server(
        1234, 3, 60000, false,             /* 端口 ET模式 timeoutMs 优雅退出  */
        3306, "root", "", "yourdb", /* Mysql配置 */
        6, 8, flag, 1, i);             /* 连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量 */
    server.Start();
} 
  