#include "Utils.hpp"
#include "RequestParser.hpp"
#include <stdlib.h>

RequestParser::RequestParser()
{
    _parsingIndex = 0;
}

RequestParser::~RequestParser()
{
}

SRet<bool> RequestParser::parseRequest(std::string request, ConfigParser config)
{
    (void)config;
    size_t endOfRequestLine = request.find("\r\n");
    if (endOfRequestLine == std::string::npos)
        return SRet<bool>(EXIT_FAILURE, false, "Invalid request");
    SRet<bool> requestLineRet = parseRequestLine(request.substr(0, endOfRequestLine));
    if (requestLineRet.status == EXIT_FAILURE)
        return requestLineRet;

    size_t endOfHeaders = request.find("\r\n\r\n");
    if (endOfHeaders == std::string::npos)
        return SRet<bool>(EXIT_FAILURE, false, "Invalid request");
    SRet<bool> headersRet = parseHeaders(request.substr(endOfRequestLine + 2, endOfHeaders - endOfRequestLine - 2));
    if (headersRet.status == EXIT_FAILURE)
        return headersRet;

    size_t endOfBody = request.find("\r\n\r\n");
    if (endOfBody == std::string::npos)
        return SRet<bool>(EXIT_FAILURE, false, "Invalid request");
    SRet<bool> bodyRet = parseBody(request.substr(endOfHeaders + 4));
    if (bodyRet.status == EXIT_FAILURE)
        return bodyRet;

    // PRINT ALL THE PARSED DATA
    std::cout << "Method: " << _method << std::endl;
    std::cout << "URI: " << _uri << std::endl;
    std::cout << "Path: " << _path << std::endl;
    std::cout << "HTTP Version: " << _httpVersion << std::endl;
    std::cout << "Body: " << _body << std::endl;
    std::map<std::string, std::string>::iterator it;
    for (it = _headers.begin(); it != _headers.end(); it++)
    {
        std::cout << "Header: " << it->first << " = " << it->second << std::endl;
    }
    for (it = _params.begin(); it != _params.end(); it++)
    {
        std::cout << "Param: " << it->first << " = " << it->second << std::endl;
    }
    for (it = _cookies.begin(); it != _cookies.end(); it++)
    {
        std::cout << "Cookie: " << it->first << " = " << it->second << std::endl;
    }


    return SRet<bool>(EXIT_SUCCESS, true);
}

SRet<bool> RequestParser::parseRequestLine(std::string requestLine)
{
    size_t firstSpace = requestLine.find(" ");
    if (firstSpace == std::string::npos)
        return SRet<bool>(EXIT_FAILURE, false, "Invalid request line");
    _method = requestLine.substr(0, firstSpace);
    size_t secondSpace = requestLine.find(" ", firstSpace + 1);
    if (secondSpace == std::string::npos)
        return SRet<bool>(EXIT_FAILURE, false, "Invalid request line");
    
    _uri = requestLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    size_t questionMark = _uri.find("?");
    if (questionMark == std::string::npos)
        _path = _uri;
    else
    {
        _path = _uri.substr(0, questionMark);
        SRet<bool> paramsRet = parseParams(_uri.substr(questionMark + 1));
        if (paramsRet.status == EXIT_FAILURE)
            return paramsRet;
    }
    size_t httpVersionStart = requestLine.find("HTTP/");
    if (httpVersionStart == std::string::npos)
        return SRet<bool>(EXIT_FAILURE, false, "Invalid request line");
    _httpVersion = requestLine.substr(httpVersionStart + 5);
    return SRet<bool>(EXIT_SUCCESS, true);
}

SRet<bool> RequestParser::parseHeaders(std::string headers)
{
    std::vector<std::string> headerLines = split(headers, '\n');
    for (size_t i = 0; i < headerLines.size(); i++)
    {
        size_t colonPos = headerLines[i].find(":");
        if (colonPos == std::string::npos)
            return SRet<bool>(EXIT_FAILURE, false, "Invalid header line");
        std::string key = headerLines[i].substr(0, colonPos);
        std::string value = headerLines[i].substr(colonPos + 1);
        _headers[key] = value;

        if (key == "Cookie")
        {
            SRet<bool> cookiesRet = parseCookies(value);
            if (cookiesRet.status == EXIT_FAILURE)
                return cookiesRet;
        }
    }
    return SRet<bool>(EXIT_SUCCESS, true);
}

SRet<bool> RequestParser::parseBody(std::string body)
{
    _body = body;
    return SRet<bool>(EXIT_SUCCESS, true);
}

SRet<bool> RequestParser::parseParams(std::string params)
{
    std::vector<std::string> paramPairs = split(params, '&');
    for (size_t i = 0; i < paramPairs.size(); i++)
    {
        size_t equalPos = paramPairs[i].find("=");
        if (equalPos == std::string::npos)
            return SRet<bool>(EXIT_FAILURE, false, "Invalid param pair");
        std::string key = paramPairs[i].substr(0, equalPos);
        std::string value = paramPairs[i].substr(equalPos + 1);
        _params[key] = value;
    }
    return SRet<bool>(EXIT_SUCCESS, true);
}

SRet<bool> RequestParser::parseCookies(std::string cookies)
{
    std::vector<std::string> cookiePairs = split(cookies, ';');
    for (size_t i = 0; i < cookiePairs.size(); i++)
    {
        size_t equalPos = cookiePairs[i].find("=");
        if (equalPos == std::string::npos)
            return SRet<bool>(EXIT_FAILURE, false, "Invalid cookie pair");
        std::string key = cookiePairs[i].substr(0, equalPos);
        std::string value = cookiePairs[i].substr(equalPos + 1);
        _cookies[key] = value;
    }
    return SRet<bool>(EXIT_SUCCESS, true);
}