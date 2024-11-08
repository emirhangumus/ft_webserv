#include "Utils.hpp"
#include "RequestParser.hpp"
#include "ErrorResponse.hpp"
#include "CGIRunner.hpp"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <dirent.h>

#include <cstring>
#include <netdb.h>
#include <arpa/inet.h>

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

    // // PRINT ALL THE PARSED DATA
    // std::cout << "Method: " << _method << std::endl;
    // std::cout << "URI: " << _uri << std::endl;
    // std::cout << "Path: " << _path << std::endl;
    // std::cout << "HTTP Version: " << _httpVersion << std::endl;
    // std::cout << "Body: " << _body << std::endl;
    // std::map<std::string, std::string>::iterator it;
    // for (it = _headers.begin(); it != _headers.end(); it++)
    // {
    //     std::cout << "Header: " << it->first << " = " << it->second << std::endl;
    // }
    // for (it = _params.begin(); it != _params.end(); it++)
    // {
    //     std::cout << "Param: " << it->first << " = " << it->second << std::endl;
    // }
    // for (it = _cookies.begin(); it != _cookies.end(); it++)
    // {
    //     std::cout << "Cookie: " << it->first << " = " << it->second << std::endl;
    // }

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
    {
        _path = _uri;
        // normalize the path. example: "/path/to/file/" -> "/path/to/file" or "/path/to/file////" -> "/path/to/file"
        _path = _path.substr(0, _path.find_last_not_of("/") + 1);
    }
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

std::string localhostToIp(const std::string& host)
{
    // Split the host string into hostname and port
    size_t colon_pos = host.find(':');
    std::string hostname = host;
    std::string port = "";
    if (colon_pos != std::string::npos) {
        hostname = host.substr(0, colon_pos);
        port = host.substr(colon_pos + 1);
    }

    // Set up address info
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4 addresses
    hints.ai_socktype = SOCK_STREAM;

    // Get address info for the hostname
    if (getaddrinfo(hostname.c_str(), NULL, &hints, &res) != 0) {
        std::cerr << "Error getting address info for " << hostname << std::endl;
        return "";
    }

    // Convert the IP address to a string
    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
    inet_ntop(AF_INET, &ipv4->sin_addr, ip, sizeof(ip));

    freeaddrinfo(res); // Free the allocated memory

    // Append port if it was provided
    std::string result = std::string(ip);
    if (!port.empty()) {
        result += ":" + port;
    }

    return result;
}

SRet<std::string> RequestParser::prepareResponse()
{
    // check the rules via the config

    // find the `Host`
    std::map<std::string, std::string>::iterator it = _headers.find("Host");
    if (it == _headers.end())
        return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(400));

    // find the corrent config
    std::string host = localhostToIp(trim(it->second));
    Config serverConfig = config.getServerConfig(host);
    if (serverConfig.getPort() == 0)
        return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(400));

    serverConfig.printConfig();

    Location loc = serverConfig.getCorrentLocation(_path);

    // check the method
    if (std::find(loc.getAllowMethods().begin(), loc.getAllowMethods().end(), _method) == loc.getAllowMethods().end())
        return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(405));

    // check the body size
    if (loc.getClientMaxBodySize() != -1 && compareSizeTandLongLong(_body.size(), loc.getClientMaxBodySize()))
        return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(413));

    // is it a CGI request?
    if (loc.getCgiParams().size() != 0)
    {
        CGIRunner cgiRunner = CGIRunner(loc);
        return cgiRunner.runCGI(_path, _params, _method, _cookies, _body);
    }
    else
    {
        // is autoindex on?
        if (loc.getAutoindex() == "on")
        {
            // open the directory
            std::string root = loc.getRoot();
            std::string path = root + _path;
            DIR *dir = opendir(path.c_str());
            if (dir == NULL)
            {
                std::cout << "Pathxxxxxxx: " << path << std::endl;
                // is this a file?
                struct stat buffer;
                if (stat(path.c_str(), &buffer) == 0)
                {
                    if (S_ISREG(buffer.st_mode))
                    {
                        // open the file
                        std::ifstream file(path.c_str());
                        if (!file.is_open())
                            return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(404));

                        // read the file
                        std::string response;
                        std::string line;
                        while (std::getline(file, line))
                            response += line + "\n";
                        file.close();

                        // find the content-type (html)
                        std::string contentType = "text/html";

                        // prepare the response
                        response = "HTTP/1.1 200 OK\r\nContent-Type: " + contentType + "\r\nContent-Length: " + size_tToString(response.size()) + "\r\n\r\n" + response;
                        return SRet<std::string>(EXIT_SUCCESS, response);
                    }
                    else
                        return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(404));
                }
                else
                    return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(404));
            }
            else
            {
                // read the directory
                std::string response = "<html><head><title>Index of " + _path + "</title></head><body><h1>Index of " + _path + "</h1><hr><pre>";
                struct dirent *ent;
                while ((ent = readdir(dir)) != NULL)
                {
                    response += "<a href=\"" + _path + "/" + ent->d_name + "\">" + ent->d_name + "</a>\n";
                }
                closedir(dir);

                // prepare the response
                response += "</pre><hr></body></html>";
                response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + size_tToString(response.size()) + "\r\n\r\n" + response;
                return SRet<std::string>(EXIT_SUCCESS, response);
            }
        } 
        else {
            // goto the root directory and search the 'index' file
            std::string index = loc.getIndex();
            std::string root = loc.getRoot();
            std::string path = root + _path;
            std::cout << "Path: " << path << std::endl;
            struct stat buffer;
            if (stat(path.c_str(), &buffer) == 0)
            {
                if (S_ISDIR(buffer.st_mode))
                {
                    if (index == "")
                        return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(404));
                    path += "/" + index;
                }
            }
            else
                return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(404));

            // open the file
            std::ifstream file(path.c_str());
            if (!file.is_open())
                return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(404));

            // read the file
            std::string response;
            std::string line;
            while (std::getline(file, line))
                response += line + "\n";
            file.close();

            // find the content-type (html)
            std::string contentType = "text/html";

            // prepare the response
            response = "HTTP/1.1 200 OK\r\nContent-Type: " + contentType + "\r\nContent-Length: " + size_tToString(response.size()) + "\r\n\r\n" + response;
            return SRet<std::string>(EXIT_SUCCESS, response);
        }
    }
}
