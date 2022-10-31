#pragma once
#include <unordered_map>
#include <fcntl.h>    // open
#include <unistd.h>   // close
#include <sys/stat.h> // stat
#include <sys/mman.h> // mmap, munmap

#include "../src/log.h"
#include "../src/simvector.h"
#include "../src/LRUCache.h"
#include "httprequest.h"

class HttpResponse : public HttpRequest
{
protected:
    char *mmFile_;
    struct stat mmFileStat_;

private:
    int code_;
    static const std::unordered_map<std::string_view, std::string_view> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string_view> CODE_STATUS;
    static const std::unordered_map<int, std::string_view> CODE_PATH;
    static LRUCache<std::string,char *> Cache_;
    void AddStateLine_(Buff &buff);
    void AddHeader_(Buff &buff);
    void AddContent_(Buff &buff);
    void ErrorContent(Buff &buff, std::string_view message);
    void ErrorHtml_();
    std::string_view GetFileType_();

public:
    static std::string srcDir;
    HttpResponse() : HttpRequest()
    {
        code_ = 200;
        mmFile_ = nullptr;
        mmFileStat_ = {0};
    }
    ~HttpResponse()
    {  
        UnmapFile();
    }
    void ResponseClear()
    {
        if (mmFile_)
        {
            UnmapFile();
        }
        code_ = 200;
        mmFile_ = nullptr;
        mmFileStat_ = {0};
    }
    void MakeResponse(Buff &buff);
    void UnmapFile();

    int Code() const { return code_; }
};