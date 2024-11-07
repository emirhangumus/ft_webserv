#include "Utils.hpp"
#include "RequestParser.hpp"
#include "ErrorResponse.hpp"
#include <stdlib.h>

RequestParser::RequestParser(ConfigParser config)
{
    _parsingIndex = 0;
    this->config = config;
    _isParsed = false;
}

RequestParser::~RequestParser()
{
}

SRet<bool> RequestParser::parseRequest(std::string request)
{
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

    _isParsed = true;
    return SRet<bool>(EXIT_SUCCESS, true);
}

SRet<bool> RequestParser::parseRequestLine(std::string requestLine)
{
    // if there is not two spaces in the request line, it is invalid
    if (countThis(requestLine, ' ') != 2)
        return SRet<bool>(EXIT_FAILURE, false, "Invalid request line");

    size_t firstSpace = requestLine.find(" ");
    if (firstSpace == std::string::npos)
        return SRet<bool>(EXIT_FAILURE, false, "Invalid request line");
    _method = requestLine.substr(0, firstSpace);

    // TODO: CHECK THE CONFIG AND SEE IF THE METHOD IS ALLOWED

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
        if (cookiePairs[i][0] != ' ')
            return SRet<bool>(EXIT_FAILURE, false, "Invalid cookie pair");
        std::string key = cookiePairs[i].substr(1, equalPos - 1);
        std::string value = cookiePairs[i].substr(equalPos + 1);
        _cookies[key] = value;
    }
    return SRet<bool>(EXIT_SUCCESS, true);
}

SRet<std::string> RequestParser::prepareResponse()
{
    // check the rules via the config

    // find the `Host`
    std::map<std::string, std::string>::iterator it = _headers.find("Host");
    if (it == _headers.end())
        return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(400));

    // find the corrent config
    std::string host = it->second;
    Config serverConfig = config.getServerConfig(host);
    if (serverConfig.getPort() == 0)
        return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(400));

    // check the method
    // std::map<std::string, Location> locations = serverConfig.getLocations();
    // std::map<std::string, Location>::iterator locIt = locations.find(_path);
    // if (locIt == locations.end())
    //     return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(404));

    Location loc = serverConfig.getCorrentLocation(_path);

    // print the location
    loc.printLocation();

    Location mainLoc = serverConfig.getMainLocation();

    std::cout << "Main Location: " << std::endl;
    mainLoc.printLocation();

    // check the method
    // std::vector<std::string> allowMethods = locIt->second.getAllowMethods();
    
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\nAccess-Control-Allow-Headers: *\r\n\r\nHello World!";
    return SRet<std::string>(EXIT_SUCCESS, response); 
}
