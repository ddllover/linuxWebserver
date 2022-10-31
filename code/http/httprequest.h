#pragma once
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>
#include <mysql/mysql.h> //mysql

#include "../src/simvector.h"
#include "../src/log.h"
#include "../sql/sqlconnpool.h"

class HttpRequest
{
protected:
    bool isKeepAlive_;
    std::string path_, body_;
    std::string_view method_, version_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;

private:
    enum PARSE_STATE
    {
        REQUEST_LINE,
        HEADERS,
        FINISH,
    } state_;
    static const std::unordered_set<std::string_view> DEFAULT_HTML;

    bool ParseHeader_(Buff &buff);
    bool ParseBody_(Buff &buff);

    void ParsePath_();
    void ParsePost_();
    void ParseFromUrlencoded_();

    static bool UserVerify(const std::string &name, const std::string &pwd, bool isLogin);

public:
    HttpRequest() { RequestClear(); }
    ~HttpRequest() {};
    void RequestClear()
    {
        isKeepAlive_ = false;
        method_ = path_ = version_ = body_ = "";
        state_ = REQUEST_LINE;
        header_.clear();
        post_.clear();
    }
    bool ParseRequest(Buff &buff);
};