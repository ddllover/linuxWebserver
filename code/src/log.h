#pragma once
#include <mutex>
#include <condition_variable>
#include <string>
#include <thread>
#include <cstring>
#include <functional>
#include <chrono>
#include <time.h>
#include <stdarg.h> // vastart va_end
#include <assert.h>
#include <sys/stat.h> //mkdir
#include <fcntl.h>    //open
#include <unistd.h>   //read write

#include "../src/simvector.h"

// 采用宏定义并不是因为变参，而是用来过滤
#define LOG_BASE(level, format, ...)                                      \
    {                                                                     \
        Log &ptr = Log::getLog();                                         \
        if (ptr.isOpen() && ptr.getLevel() <= level)                      \
        {                                                                 \
            ptr.append(level, __DATE__, __TIME__, format, ##__VA_ARGS__); \
        }                                                                 \
    }

#define LOG_DEBUG(format, ...) LOG_BASE(0, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) LOG_BASE(1, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) LOG_BASE(2, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) LOG_BASE(3, format, ##__VA_ARGS__)

class Log
{ // 做个单例
private:
    static const int MAX_LINES = 500000;
    std::mutex logmutex_;
    std::condition_variable logcond_;
    int maxBufsize_;
    std::unique_ptr<Buff> crubuf_;
    std::unique_ptr<Buff> nexbuf_;
    std::unique_ptr<std::thread> logthread_;

    int filefd_ = -1;
    char path_[50] = {0};         // 文件路径
    const char *suffix_ = ".log"; // 文件后缀
    int level_;
    int lineCount_ = 0;
    int tDay_ = -1; // 按天来分级
    bool isOpen_ = false;
    Log() {}
    Log(const Log &) = delete;
    Log(Log &&) = delete;
    Log &operator=(const Log &) = delete;
    Log &operator=(Log &&) = delete;
    class Logwork
    {
    private:
        Log *ptr = nullptr;

    public:
        Logwork(Log *obj) { ptr = obj; }
        void operator()()
        {
            std::unique_lock lk(ptr->logmutex_);
            while (ptr->isOpen_)
            {
                if (ptr->crubuf_->size() > ptr->maxBufsize_)
                {
                    std::swap(ptr->crubuf_, ptr->nexbuf_);
                    lk.unlock();
                    ptr->updateFile();
                    if (write(ptr->filefd_, ptr->nexbuf_->data(), ptr->nexbuf_->size()) < 0)
                    {
                        LOG_ERROR("log write %s\n", strerror(errno));
                    }
                    if (ptr->nexbuf_->capacity() > 1.5 * ptr->maxBufsize_)
                    {
                        ptr->nexbuf_->shift_to(ptr->maxBufsize_);
                    }
                    ptr->nexbuf_->clear();
                    lk.lock();
                }
                else
                {
                    auto now = std::chrono::system_clock::now() + std::chrono::duration<double>(60);
                    ptr->logcond_.wait_until(lk, now);
                }
            }
        }
    };
    void updateFile()
    {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        struct tm time_ = *localtime(&now);
        if (tDay_ != time_.tm_mday || (lineCount_ && (lineCount_ > MAX_LINES)))
        {
            lineCount_ = 0;
            if (filefd_ > 0)
                close(filefd_);
            static int cnt = 0;
            char newFile[256] = {0};
            char tail[36] = {0};
            sprintf(tail, "%04d_%02d_%02d", time_.tm_year + 1900, time_.tm_mon + 1, time_.tm_mday);
            if (tDay_ != time_.tm_mday)
            {
                tDay_ = time_.tm_mday;
                if (filefd_ < 0) // 第一次
                {
                    do
                    {
                        sprintf(newFile, "%s/%s-%d%s", path_, tail, cnt++, suffix_);
                    } while (access(newFile, F_OK) == 0);
                }
                else
                { // 过了一天
                    cnt = 0;
                    sprintf(newFile, "%s/%s-%d%s", path_, tail, cnt++, suffix_);
                }
            }
            else
            {
                sprintf(newFile, "%s/%s-%d%s", path_, tail, cnt++, suffix_);
            } // 立即读写，缓冲区自己控制
            filefd_ = open(newFile, O_RDWR | O_CREAT | O_APPEND | O_NDELAY | O_SYNC, 00777);
            if (filefd_ < 0)
            {
                mkdir(path_, 00777);
                filefd_ = open(newFile, O_RDWR | O_CREAT | O_APPEND | O_NDELAY | O_SYNC, 00777);
                if (filefd_ < 0)
                {
                    LOG_ERROR("file %s", strerror(errno));
                }
            }
        }
    }

public:
    ~Log() { shutdown(); }
    void shutdown()
    {
        {
            std::lock_guard lk(logmutex_);
            isOpen_ = false;
        }
        logcond_.notify_one();
        if (logthread_->joinable())
        {
            logthread_->join();
        }
        if (filefd_ > 0)
            close(filefd_);
    }
    void Init(int level = 4, std::string path = "./log", int buflen = 100000)
    {
        if (isOpen_)
            shutdown();

        isOpen_ = true;
        level_ = level;
        strcpy(path_, path.data());
        maxBufsize_ = buflen;
        nexbuf_ = std::make_unique<Buff>(buflen);
        crubuf_ = std::make_unique<Buff>(buflen);
        updateFile();
        logthread_ = std::make_unique<std::thread>(Logwork(this));
        LOG_INFO("LogSys level: %d  buflen: %d", level,buflen);
    }
    static Log &getLog()
    {
        static Log ptr_;
        return ptr_;
    }
    bool isOpen() const
    {
        return isOpen_;
    }
    int getLevel() const
    {
        return level_;
    }
    void append(int level, const char *date, const char *time, const char *format, ...)
    {

        va_list vaList;
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        struct tm time_ = *localtime(&now);
        {
            std::lock_guard lk(logmutex_);
            lineCount_++;
            crubuf_->reserve(crubuf_->size() + 128);
            int n = snprintf(crubuf_->end(), 128, "%d-%02d-%02d %02d:%02d:%02d",
                             time_.tm_year + 1900, time_.tm_mon + 1, time_.tm_mday,
                             time_.tm_hour, time_.tm_min, time_.tm_sec);
            crubuf_->resize(crubuf_->size() + n);
            switch (level)
            {
            case 0:
                crubuf_->Append("[debug]: ", 9);
                break;
            case 1:
                crubuf_->Append("[info] : ", 9);
                break;
            case 2:
                crubuf_->Append("[warn] : ", 9);
                break;
            case 3:
                crubuf_->Append("[error]: ", 9);
                break;
            default:
                crubuf_->Append("[info] : ", 9);
                break;
            }

            va_start(vaList, format);
            int m = vsnprintf(crubuf_->end(), crubuf_->leftsize(), format, vaList);
            va_end(vaList);
            crubuf_->resize(crubuf_->size() + m);
            crubuf_->Append("\n", 1);
            if (crubuf_->size() > maxBufsize_)
                logcond_.notify_one();
        }
        // crubuf_->Append(str); //字符串格式
    }
};
