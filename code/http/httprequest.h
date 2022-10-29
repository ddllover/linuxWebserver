#pragma once
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>
#include <mysql/mysql.h> //mysql

#include "../src/simvector.h"
#include "../src/log.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"

class HttpRequest
{
protected:
    bool isKeepAlive_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;
private:
    enum PARSE_STATE
    {
        REQUEST_LINE,
        HEADERS,
        FINISH,
    }state_;
    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;

    bool ParseHeader_(Buff &buff);
    bool ParseBody_(Buff &buff);

    void ParsePath_();
    void ParsePost_();
    void ParseFromUrlencoded_();

    static bool UserVerify(const std::string &name, const std::string &pwd, bool isLogin);

public:
    HttpRequest() { RequestClear(); }
    ~HttpRequest() = default;
    void RequestClear();
    bool ParseRequest(Buff &buff);
};