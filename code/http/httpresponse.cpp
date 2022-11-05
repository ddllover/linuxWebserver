#include "httpresponse.h"

using namespace std;
string HttpResponse::srcDir;
LRUCache<string, char *> HttpResponse::Cache_; // 10M
const unordered_map<string_view, string_view> HttpResponse::SUFFIX_TYPE = {
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css "},
    {".js", "text/javascript "},
};

const unordered_map<int, string_view> HttpResponse::CODE_STATUS = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

const unordered_map<int, string_view> HttpResponse::CODE_PATH = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

void HttpResponse::MakeResponse(Buff &buff)
{
    /* 判断请求的资源文件 */
    if (stat((srcDir + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode))
    {
        code_ = 404;
    }
    else if (!(mmFileStat_.st_mode & S_IROTH))
    {
        code_ = 403;
    }
    else if (code_ == -1)
    {
        code_ = 200;
    }
    ErrorHtml_();
    AddStateLine_(buff);
    AddHeader_(buff);
    AddContent_(buff);
}

void HttpResponse::ErrorHtml_()
{
    if (CODE_PATH.find(code_) != CODE_PATH.end())
    {
        path_ = CODE_PATH.find(code_)->second;
        stat((srcDir + path_).data(), &mmFileStat_);
    }
}

void HttpResponse::AddStateLine_(Buff &buff)
{
    string status;
    if (CODE_STATUS.find(code_) != CODE_STATUS.end())
    {
        status = CODE_STATUS.find(code_)->second;
    }
    else
    {
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff.append("HTTP/1.1 " + to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::AddHeader_(Buff &buff)
{
    buff.append("Connection: ");
    if (isKeepAlive_)
    {
        buff.append("keep-alive\r\n");
        buff.append("keep-alive: max=6, timeout=120\r\n");
    }
    else
    {
        buff.append("close\r\n");
    }
    buff.append("Content-type: ");
    buff.append(string(GetFileType_()));
    buff.append("\r\n");
}

void HttpResponse::AddContent_(Buff &buff)
{   
    char * file;
    if (Cache_.find(path_,&file))
    {
        mmFile_ = file;
        LOG_DEBUG("%s Cached hit",path_.data());
    }
    else
    {
        int srcFd = open((srcDir + path_).data(), O_RDONLY);
        if (srcFd < 0)
        {
            ErrorContent(buff, "File NotFound!");
            return;
        }

        /* 将文件映射到内存提高文件的访问速度
            MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
        LOG_DEBUG("file path %s", path_.data());
        int *mmRet = (int *)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
        if (*mmRet == -1)
        {
            ErrorContent(buff, "File NotFound!");
            return;
        }
        mmFile_ = (char *)mmRet;
        close(srcFd);
        // 添加到缓存
        Cache_.put(path_, mmFile_, mmFileStat_.st_size);
        //LOG_DEBUG("%s add cache",path_.data());
    }
    buff.append("Content-length: ");
    buff.append(to_string(mmFileStat_.st_size));
    buff.append("\r\n\r\n");
}

void HttpResponse::UnmapFile()
{
    if (mmFile_)
    {
        if (!Cache_.find(path_))
        {   LOG_DEBUG("%s Cache replace",path_.data());
            munmap(mmFile_, mmFileStat_.st_size);
        }   
    }
    mmFile_ = nullptr;
    memset(&mmFileStat_,0,sizeof(mmFileStat_));
}

string_view HttpResponse::GetFileType_()
{
    /* 判断文件类型 */
    string::size_type idx = path_.find_last_of('.');
    if (idx == string::npos)
    {
        return "text/plain";
    }
    string suffix = path_.substr(idx);
    if (SUFFIX_TYPE.count(suffix) == 1)
    {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

void HttpResponse::ErrorContent(Buff &buff, string_view message)
{
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if (CODE_STATUS.count(code_) == 1)
    {
        status = CODE_STATUS.find(code_)->second;
    }
    else
    {
        status = "Bad Request";
    }
    body += to_string(code_) + " : " + status + "\n";
    body += "<p>" + string(message.data()) + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buff.append(body);
}
