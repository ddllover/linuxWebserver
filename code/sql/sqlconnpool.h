#pragma once
#include <iostream>
#include <mysql/mysql.h>
#include <cstring>
#include <queue>
#include <mutex>
#include <thread>
#include <semaphore.h>
#include "../src/safequeue.h"
#include "../src/log.h"
class SqlConnPool
{

private:
    SqlConnPool()
    {
        useCount_ = 0;
        freeCount_ = 0;
    }
    int MAX_CONN_;
    int useCount_;
    int freeCount_;

    SafeQueue<MYSQL *> connQue_;
    //std::counting_semaphore cnt_;
    sem_t semId_;

public:
    ~SqlConnPool()
    {
        ClosePool();
    }
    static SqlConnPool *Instance()
    {
        static SqlConnPool connPool;
        return &connPool;
    }
    void Init(const char *host, int port, const char *user, const char *pwd, const char *dbName, int connSize = 6)
    {
        assert(connSize > 0);
        for (int i = 0; i < connSize; i++)
        {
            MYSQL *sql = nullptr;
            sql = mysql_init(sql);
            assert(sql);
            sql = mysql_real_connect(sql, host, user, pwd,
                                     dbName, port, nullptr, 0);
            assert(sql);
            connQue_.push(sql);
        }
        
        MAX_CONN_ = connSize;
        sem_init(&semId_, 0, MAX_CONN_);
    }
    void ClosePool()
    {
        while (!connQue_.empty())
        {
            auto item = connQue_.front();
            connQue_.pop();
            mysql_close(item);
        }
        mysql_library_end();
    }
    MYSQL *GetConn()
    {
        MYSQL *sql = nullptr;
        if (connQue_.empty())
        {
            return nullptr;
        }
        sem_wait(&semId_);
        {
            sql = connQue_.front();
            connQue_.pop();
        }
        return sql;
    }
    void FreeConn(MYSQL *sql)
    {
        assert(sql);
        connQue_.push(sql);
        sem_post(&semId_);
    }
    int GetFreeConnCount()
    {
        return connQue_.size();
    }
};
class SqlConnRAII
{
public:
    SqlConnRAII(MYSQL **sql, SqlConnPool *connpool)
    {
        assert(connpool);
        *sql = connpool->GetConn();
        sql_ = *sql;
        connpool_ = connpool;
    }

    ~SqlConnRAII()
    {
        if (sql_)
        {
            connpool_->FreeConn(sql_);
        }
    }

private:
    MYSQL *sql_;
    SqlConnPool *connpool_;
};