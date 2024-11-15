#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include "Utils.hpp"
#include "ConfigParser.hpp"
#include "MimeTypes.hpp"
#include <string>
#include <iostream>
#include <map>
#include <vector>

class RequestParser
{
public:
    RequestParser(ConfigParser config);
    ~RequestParser();
    SRet<bool> parseRequest(std::string request);
    SRet<std::string> prepareResponse(MimeTypes& mimeTypes);

    std::string getMethod() { return _method; }
    std::string getUri() { return _uri; }
    std::string getPath() { return _path; }
    std::string getHttpVersion() { return _httpVersion; }
    std::string getBody() { return _body; }
    std::map<std::string, std::string> getHeaders() { return _headers; }
    std::map<std::string, std::string> getParams() { return _params; }
    std::map<std::string, std::string> getCookies() { return _cookies; }
private:
    ConfigParser config;
    bool _isParsed;

    std::string _method;
    std::string _uri;
    std::string _path;
    std::string _httpVersion;
    std::string _body;
    std::map<std::string, std::string> _headers;
    std::map<std::string, std::string> _params;
    std::map<std::string, std::string> _cookies;

    unsigned int _parsingIndex; // i guess, this is not needed rn.

    SRet<bool> parseRequestLine(std::string requestLine);
    SRet<bool> parseHeaders(std::string headers);
    SRet<bool> parseBody(std::string body);
    SRet<bool> parseParams(std::string params);
    SRet<bool> parseCookies(std::string cookies);
};

#endif