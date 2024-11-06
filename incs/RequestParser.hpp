#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include "Utils.hpp"
#include "ConfigParser.hpp"
#include <string>
#include <iostream>
#include <map>
#include <vector>

class RequestParser
{
public:
    RequestParser();
    ~RequestParser();
    SRet<bool> parseRequest(std::string request, ConfigParser config);
private:
    std::string _method;
    std::string _uri;
    std::string _path;
    std::string _httpVersion;
    std::string _body;
    std::map<std::string, std::string> _headers;
    std::map<std::string, std::string> _params;
    std::map<std::string, std::string> _cookies;

    unsigned int _parsingIndex;

    SRet<bool> parseRequestLine(std::string requestLine);
    SRet<bool> parseHeaders(std::string headers);
    SRet<bool> parseBody(std::string body);
    SRet<bool> parseParams(std::string params);
    SRet<bool> parseCookies(std::string cookies);
};

#endif