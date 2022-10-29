/*
 * @Author       : mark
 * @Date         : 2020-06-20
 * @copyleft Apache 2.0
 */
#include<iostream>
#include "../code/src/log.h"
#include "../code/pool/threadpool.h"

void TestLog()
{
    int cnt = 0, level = 0;
    Log::getLog().Init(level,"./test/log");
    for (int j = 0; j < 10000; j++)
    {
        for (int i = 0; i < 4; i++)
        {
            LOG_BASE(i, "%s 111111111 %d ============= ", "Test", cnt++);
        }
    }
    //Log::getLog().shutdown();
}

void ThreadLogTask(int i, int cnt)
{
    for (int j = 0; j < 10000; j++)
    {
        LOG_BASE(i, "PID:[%p]======= %05d ========= ",std::this_thread::get_id(), cnt++);
    }
}

void TestThreadPool()
{
    Log::getLog().Init(0,"./test/log");
    ThreadPool threadpool(6);
    for (int i = 0; i < 18; i++)
    {
        threadpool.AddTask(std::bind(ThreadLogTask, i % 4, i * 10000));
    }
    getchar();
    //Log::getLog().shutdown();
}
int main()
{
    TestLog();
    TestThreadPool();

}