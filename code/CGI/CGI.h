#pragma once
#include <iostream>
#include <stdlib.h> // atoi()
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <mysql/mysql.h>
#include <sys/types.h>
#include <sys/uio.h>   // readv/writev
#include <arpa/inet.h> // sockaddr_in
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap
#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"

using namespace std;
class HttpConn
{
private:
    const static std::unordered_set<std::string> DEFAULT_HTML;
    const static std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    const static  std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    const static  std::unordered_map<int, std::string> CODE_STATUS;
    const static  std::unordered_map<int, std::string> CODE_PATH;
    enum PARSE_STATE
    {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };
    enum HTTP_CODE
    {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };
    int code_;
    bool isKeepAlive_;

    //std::string path_;
    std::string srcDir_;

    char *mmFile_;
    struct stat mmFileStat_;

    int fd_ = -1;
    struct sockaddr_in addr_;

    bool isClose_ = true;

    int iovCnt_;
    struct iovec iov_[2];

    Buffer readBuff_;  // 读缓冲区
    Buffer writeBuff_; // 写缓冲区
    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;

    bool parse(Buffer &buff)
    {
        const char CRLF[] = "\r\n";
        if (buff.ReadableBytes() <= 0)
        {
            return false;
        }
        while (buff.ReadableBytes() && state_ != FINISH)
        {
            const char *lineEnd = std::search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);
            std::string line(buff.Peek(), lineEnd);
            switch (state_)
            {
            case REQUEST_LINE:
                if (!ParseRequestLine_(line))
                {
                    return false;
                }
                ParsePath_();
                break;
            case HEADERS:
                ParseHeader_(line);
                if (buff.ReadableBytes() <= 2)
                {
                    state_ = FINISH;
                }
                break;
            case BODY:
                ParseBody_(line);
                break;
            default:
                break;
            }
            if (lineEnd == buff.BeginWrite())
            {
                break;
            }
            buff.RetrieveUntil(lineEnd + 2);
        }
        LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
        return true;
    }

    void ParsePath_()
    {
        if (path_ == "/")
        {
            path_ = "/index.html";
        }
        else
        {
            for (auto &item : DEFAULT_HTML)
            {
                if (item == path_)
                {
                    path_ += ".html";
                    break;
                }
            }
        }
    }

    bool ParseRequestLine_(const std::string &line)
    {
        std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
        std::smatch subMatch;
        if (std::regex_match(line, subMatch, patten))
        {
            method_ = subMatch[1];
            path_ = subMatch[2];
            version_ = subMatch[3];
            state_ = HEADERS;
            return true;
        }
        LOG_ERROR("RequestLine Error");
        return false;
    }

    void ParseHeader_(const std::string &line)
    {
        std::regex patten("^([^:]*): ?(.*)$");
        std::smatch subMatch;
        if (std::regex_match(line.begin() + 0, line.end(), subMatch, patten))
        {
            header_[subMatch[1]] = subMatch[2];
        }
        else
        {
            state_ = BODY;
        }
    }

    void ParseBody_(const string &line)
    {
        body_ = line;
        ParsePost_();
        state_ = FINISH;
        LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
    }

    int ConverHex(char ch)
    {
        if (ch >= 'A' && ch <= 'F')
            return ch - 'A' + 10;
        if (ch >= 'a' && ch <= 'f')
            return ch - 'a' + 10;
        return ch;
    }

    void ParsePost_()
    {
        if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded")
        {
            ParseFromUrlencoded_();
            if (DEFAULT_HTML_TAG.count(path_))
            {
                int tag = DEFAULT_HTML_TAG.find(path_)->second;
                LOG_DEBUG("Tag:%d", tag);
                if (tag == 0 || tag == 1)
                {
                    bool isLogin = (tag == 1);
                    if (UserVerify(post_["username"], post_["password"], isLogin))
                    {
                        path_ = "/welcome.html";
                    }
                    else
                    {
                        path_ = "/error.html";
                    }
                }
            }
        }
    }

    void ParseFromUrlencoded_()
    {
        if (body_.size() == 0)
        {
            return;
        }
        // username=1234&password=12345
        string key, value;
        int num = 0;
        int n = body_.size();
        int i = 0, j = 0;

        for (; i < n; i++)
        {
            char ch = body_[i];
            switch (ch)
            {
            case '=':
                key = body_.substr(j, i - j);
                j = i + 1;
                break;
            case '+':
                body_[i] = ' ';
                break;
            case '%':
                num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
                body_[i + 2] = num % 10 + '0';
                body_[i + 1] = num / 10 + '0';
                i += 2;
                break;
            case '&':
                value = body_.substr(j, i - j);
                j = i + 1;
                post_[key] = value;
                LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
                break;
            default:
                break;
            }
        }
        assert(j <= i);
        if (post_.count(key) == 0 && j < i)
        {
            value = body_.substr(j, i - j);
            post_[key] = value;
        }
    }

    bool UserVerify(const string &name, const string &pwd, bool isLogin)
    {
        if (name == "" || pwd == "")
        {
            return false;
        }
        LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
        MYSQL *sql;
        SqlConnRAII(&sql, SqlConnPool::Instance());
        assert(sql);

        bool flag = false;
        unsigned int j = 0;
        char order[256] = {0};
        MYSQL_FIELD *fields = nullptr;
        MYSQL_RES *res = nullptr;

        if (!isLogin)
        {
            flag = true;
        }
        /* 查询用户及密码 */
        snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
        LOG_DEBUG("%s", order);

        if (mysql_query(sql, order))
        {
            mysql_free_result(res);
            return false;
        }
        res = mysql_store_result(sql);
        j = mysql_num_fields(res);
        fields = mysql_fetch_fields(res);

        while (MYSQL_ROW row = mysql_fetch_row(res))
        {
            LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
            string password(row[1]);
            /* 注册行为 且 用户名未被使用*/
            if (isLogin)
            {
                if (pwd == password)
                {
                    flag = true;
                }
                else
                {
                    flag = false;
                    LOG_DEBUG("pwd error!");
                }
            }
            else
            {
                flag = false;
                LOG_DEBUG("user used!");
            }
        }
        mysql_free_result(res);

        /* 注册行为 且 用户名未被使用*/
        if (!isLogin && flag == true)
        {
            LOG_DEBUG("regirster!");
            bzero(order, 256);
            snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
            LOG_DEBUG("%s", order);
            if (mysql_query(sql, order))
            {
                LOG_DEBUG("Insert error!");
                flag = false;
            }
            flag = true;
        }
        SqlConnPool::Instance()->FreeConn(sql);
        LOG_DEBUG("UserVerify success!!");
        return flag;
    }

    void ErrorHtml_()
    {
        if (CODE_PATH.count(code_) == 1)
        {
            path_ = CODE_PATH.find(code_)->second;
            stat((srcDir_ + path_).data(), &mmFileStat_);
        }
    }

    void AddStateLine_(Buffer &buff)
    {
        string status;
        if (CODE_STATUS.count(code_) == 1)
        {
            status = CODE_STATUS.find(code_)->second;
        }
        else
        {
            code_ = 400;
            status = CODE_STATUS.find(400)->second;
        }
        buff.Append("HTTP/1.1 " + to_string(code_) + " " + status + "\r\n");
    }

    void AddHeader_(Buffer &buff)
    {
        buff.Append("Connection: ");
        if (isKeepAlive_)
        {
            buff.Append("keep-alive\r\n");
            buff.Append("keep-alive: max=6, timeout=120\r\n");
        }
        else
        {
            buff.Append("close\r\n");
        }
        buff.Append("Content-type: " + GetFileType_() + "\r\n");
    }

    void AddContent_(Buffer &buff)
    {
        int srcFd = open((srcDir_ + path_).data(), O_RDONLY);
        if (srcFd < 0)
        {
            ErrorContent(buff, "File NotFound!");
            return;
        }

        /* 将文件映射到内存提高文件的访问速度
            MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
        LOG_DEBUG("file path %s", (srcDir_ + path_).data());
        int *mmRet = (int *)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
        if (*mmRet == -1)
        {
            ErrorContent(buff, "File NotFound!");
            return;
        }
        mmFile_ = (char *)mmRet;
        close(srcFd);
        buff.Append("Content-length: " + to_string(mmFileStat_.st_size) + "\r\n\r\n");
    }

    void UnmapFile()
    {
        if (mmFile_)
        {
            munmap(mmFile_, mmFileStat_.st_size);
            mmFile_ = nullptr;
        }
    }

    string GetFileType_()
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

    void ErrorContent(Buffer &buff, string message)
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
        body += "<p>" + message + "</p>";
        body += "<hr><em>TinyWebServer</em></body></html>";

        buff.Append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
        buff.Append(body);
    }

public:
    static bool isET;
    static const char *srcDir;
    static std::atomic<int> userCount;
    HttpConn(){};
    ~HttpConn() { Close(); }
    void init(int fd, const sockaddr_in &addr) // delAndset
    {
        assert(fd > 0);
        userCount++;
        addr_ = addr;
        fd_ = fd;
        writeBuff_.RetrieveAll();
        readBuff_.RetrieveAll();
        isClose_ = false;
        // void Init()
        {
            method_ = path_ = version_ = body_ = "";
            state_ = REQUEST_LINE;
            header_.clear();
            post_.clear();
        }
        {
            code_ = -1;
        path_ = srcDir_ = "";
        isKeepAlive_ = false;
        mmFile_ = nullptr;
        mmFileStat_ = {0};
        }
        LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
    }
    ssize_t Read(int *saveErrno)
    {
        ssize_t len = -1;
        do
        {
            len = readBuff_.ReadFd(fd_, saveErrno);
            if (len <= 0)
            {
                break;
            }
        } while (isET);
        return len;
    }
    ssize_t Write(int *saveErrno)
    {
        ssize_t len = -1;
        do
        {
            len = writev(fd_, iov_, iovCnt_);
            if (len <= 0)
            {
                *saveErrno = errno;
                break;
            }
            if (iov_[0].iov_len + iov_[1].iov_len == 0)
            {
                break;
            } /* 传输结束 */
            else if (static_cast<size_t>(len) > iov_[0].iov_len)
            {
                iov_[1].iov_base = (uint8_t *)iov_[1].iov_base + (len - iov_[0].iov_len);
                iov_[1].iov_len -= (len - iov_[0].iov_len);
                if (iov_[0].iov_len)
                {
                    writeBuff_.RetrieveAll();
                    iov_[0].iov_len = 0;
                }
            }
            else
            {
                iov_[0].iov_base = (uint8_t *)iov_[0].iov_base + len;
                iov_[0].iov_len -= len;
                writeBuff_.Retrieve(len);
            }
        } while (isET || ToWriteBytes() > 10240);
        return len;
    }
    bool process()
    {
        // request_.Init();
        if (readBuff_.ReadableBytes() <= 0)
        {
            return false;
        }
        else if (parse(readBuff_))
        {
            LOG_DEBUG("%s", path_.c_str());
            Init(srcDir, path(), IsKeepAlive(), 200);
        }
        else
        {
            Init(srcDir, path(), false, 400);
        }

        MakeResponse(writeBuff_);
        /* 响应头 */
        iov_[0].iov_base = const_cast<char *>(writeBuff_.Peek());
        iov_[0].iov_len = writeBuff_.ReadableBytes();
        iovCnt_ = 1;

        /* 文件 */
        if (FileLen() > 0 && File())
        {
            iov_[1].iov_base = File();
            iov_[1].iov_len = FileLen();
            iovCnt_ = 2;
        }
        LOG_DEBUG("filesize:%d, %d  to %d", FileLen(), iovCnt_, ToWriteBytes());
        return true;
    }

    int GetFd() const
    {
        return fd_;
    };

    struct sockaddr_in GetAddr() const
    {
        return addr_;
    }

    const char *GetIP() const
    {
        return inet_ntoa(addr_.sin_addr);
    }
    
    void Close()
    {
        UnmapFile();
        if (isClose_ == false)
        {
            isClose_ = true;
            userCount--;
            close(fd_);
            LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
        }
    }
    int GetPort() const
    {
        return addr_.sin_port;
    }

    int ToWriteBytes()
    {
        return iov_[0].iov_len + iov_[1].iov_len;
    }

    bool IsKeepAlive() const
    {
        if (header_.count("Connection") == 1)
        {
            return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
        }
        return false;
    }

    std::string path() const
    {
        return path_;
    }

    std::string &path()
    {
        return path_;
    }
    std::string method() const
    {
        return method_;
    }

    std::string version() const
    {
        return version_;
    }

    std::string GetPost(const std::string &key) const
    {
        assert(key != "");
        if (post_.count(key) == 1)
        {
            return post_.find(key)->second;
        }
        return "";
    }

    std::string GetPost(const char *key) const
    {
        assert(key != nullptr);
        if (post_.count(key) == 1)
        {
            return post_.find(key)->second;
        }
        return "";
    }

    /* HttpResponse()
    {
        code_ = -1;
        path_ = srcDir_ = "";
        isKeepAlive_ = false;
        mmFile_ = nullptr;
        mmFileStat_ = {0};
    };*/

    /* ~HttpResponse()
    {
        UnmapFile();
    }*/

    void Init(const string &srcDir, string &path, bool isKeepAlive, int code)
    {
        assert(srcDir != "");
        if (mmFile_)
        {
            UnmapFile();
        }
        code_ = code;
        isKeepAlive_ = isKeepAlive;
        path_ = path;
        srcDir_ = srcDir;
        mmFile_ = nullptr;
        mmFileStat_ = {0};
    }

    void MakeResponse(Buffer &buff)
    {
        /* 判断请求的资源文件 */
        if (stat((srcDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode))
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

    char *File()
    {
        return mmFile_;
    }

    size_t FileLen() const
    {
        return mmFileStat_.st_size;
    }

    int Code() const { return code_; }
};
