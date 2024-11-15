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
#include <sstream>

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
        return SRet<bool>(EXIT_FAILURE, false, "Invalid request 1");
    SRet<bool> requestLineRet = parseRequestLine(request.substr(0, endOfRequestLine));
    if (requestLineRet.status == EXIT_FAILURE)
        return requestLineRet;

    size_t endOfHeaders = request.find("\r\n\r\n");
    if (endOfHeaders == std::string::npos)
        return SRet<bool>(EXIT_FAILURE, false, "Invalid request 2");
    SRet<bool> headersRet = parseHeaders(request.substr(endOfRequestLine + 2, endOfHeaders - endOfRequestLine - 2));
    if (headersRet.status == EXIT_FAILURE)
        return headersRet;

    size_t endOfBody = request.find("\r\n\r\n");
    if (endOfBody == std::string::npos)
        return SRet<bool>(EXIT_FAILURE, false, "Invalid request 3");
    SRet<bool> bodyRet = parseBody(request.substr(endOfHeaders + 4));
    if (bodyRet.status == EXIT_FAILURE)
        return bodyRet;

    _isParsed = true;
    return SRet<bool>(EXIT_SUCCESS, true);
}

SRet<bool> RequestParser::parseRequestLine(std::string requestLine)
{
    std::vector<std::string> ALLOWED_METHODS;
    ALLOWED_METHODS.push_back("GET");
    ALLOWED_METHODS.push_back("POST");
    ALLOWED_METHODS.push_back("DELETE");
    ALLOWED_METHODS.push_back("PUT");
    ALLOWED_METHODS.push_back("HEAD");
    ALLOWED_METHODS.push_back("CONNECT");
    ALLOWED_METHODS.push_back("OPTIONS");
    ALLOWED_METHODS.push_back("TRACE");

    if (requestLine.find('\0') != std::string::npos || requestLine.find_first_of("\r\n") != std::string::npos)
        return SRet<bool>(EXIT_FAILURE, false, "Invalid characters in request line");

    if (countThis(requestLine, ' ') != 2)
        return SRet<bool>(EXIT_FAILURE, false, "Invalid request line: wrong number of spaces");

    size_t firstSpace = requestLine.find(" ");
    if (firstSpace == std::string::npos || firstSpace == 0)
        return SRet<bool>(EXIT_FAILURE, false, "Invalid request line: no method");

    _method = requestLine.substr(0, firstSpace);

    if (std::find(ALLOWED_METHODS.begin(), ALLOWED_METHODS.end(), _method) == ALLOWED_METHODS.end())
        return SRet<bool>(EXIT_FAILURE, false, "Invalid method");

    size_t secondSpace = requestLine.find(" ", firstSpace + 1);
    if (secondSpace == std::string::npos)
        return SRet<bool>(EXIT_FAILURE, false, "Invalid request line: missing URI");

    _uri = requestLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);

    if (_uri.find("../") != std::string::npos)
        return SRet<bool>(EXIT_FAILURE, false, "Invalid URI: path traversal attempt");

    size_t questionMark = _uri.find("?");
    if (questionMark == std::string::npos)
    {
        _path = _uri;
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
        return SRet<bool>(EXIT_FAILURE, false, "Invalid request line: missing HTTP version");

    _httpVersion = requestLine.substr(httpVersionStart + 5);
    if (_httpVersion != "1.0" && _httpVersion != "1.1")
        return SRet<bool>(EXIT_FAILURE, false, "Invalid HTTP version");

    return SRet<bool>(EXIT_SUCCESS, true);
}

SRet<bool> RequestParser::parseHeaders(std::string headers)
{
    std::vector<std::string> headerLines = split(headers, '\n');

    for (size_t i = 0; i < headerLines.size(); i++)
    {

        size_t colonPos = headerLines[i].find(":");
        if (colonPos == std::string::npos || colonPos == 0 || colonPos == headerLines[i].length() - 1)
            return SRet<bool>(EXIT_FAILURE, false, "Invalid header line: missing colon or empty key/value");

        std::string key = headerLines[i].substr(0, colonPos);
        std::string value = trim(headerLines[i].substr(colonPos + 1));

        if (key.find_first_of("\0\r\n") != std::string::npos || value.find_first_of("\0\r\n") != std::string::npos)
            return SRet<bool>(EXIT_FAILURE, false, "Invalid header line: control characters detected");

        if (key.find("HTTP_") == 0 || key.find("X-") == 0 || key.find("Set-Cookie") != std::string::npos)
            return SRet<bool>(EXIT_FAILURE, false, "Invalid header name: header injection attempt detected");

        if (value.find("\r") != std::string::npos || value.find("\n") != std::string::npos)
            return SRet<bool>(EXIT_FAILURE, false, "Invalid header value: CRLF detected");

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

std::string urlDecode(const std::string &str)
{
    std::string decoded;
    size_t len = str.length();
    for (size_t i = 0; i < len; ++i)
    {
        if (str[i] == '%' && i + 2 < len)
        {

            int value = 0;
            std::stringstream hexStream;
            hexStream << str[i + 1] << str[i + 2];
            hexStream >> std::hex >> value;
            decoded.push_back(static_cast<char>(value));
            i += 2;
        }
        else if (str[i] == '+')
            decoded.push_back(' ');
        else
            decoded.push_back(str[i]);
    }
    return decoded;
}
SRet<bool> RequestParser::parseParams(std::string params)
{

    const size_t MAX_PARAM_SIZE = 1024;
    if (params.size() > MAX_PARAM_SIZE)
        return SRet<bool>(EXIT_FAILURE, false, "Request parameters too large");

    std::vector<std::string> paramPairs = split(params, '&');
    for (size_t i = 0; i < paramPairs.size(); i++)
    {
        size_t equalPos = paramPairs[i].find("=");
        if (equalPos == std::string::npos || equalPos == 0 || equalPos == paramPairs[i].length() - 1)
            return SRet<bool>(EXIT_FAILURE, false, "Invalid param pair: missing key or value");

        std::string key = paramPairs[i].substr(0, equalPos);
        std::string value = paramPairs[i].substr(equalPos + 1);

        if (key.find_first_of("\0\r\n") != std::string::npos || value.find_first_of("\0\r\n") != std::string::npos)
            return SRet<bool>(EXIT_FAILURE, false, "Invalid param pair: control characters detected");

        key = urlDecode(key);
        value = urlDecode(value);

        _params[key] = value;
    }
    return SRet<bool>(EXIT_SUCCESS, true);
}

SRet<bool> RequestParser::parseCookies(std::string cookies)
{

    const size_t MAX_COOKIE_SIZE = 4096;
    if (cookies.size() > MAX_COOKIE_SIZE)
        return SRet<bool>(EXIT_FAILURE, false, "Cookies too large");

    std::vector<std::string> cookiePairs = split(cookies, ';');
    for (size_t i = 0; i < cookiePairs.size(); i++)
    {

        std::string cookie = trim(cookiePairs[i]);

        size_t equalPos = cookie.find("=");
        if (equalPos == std::string::npos || equalPos == 0 || equalPos == cookie.length() - 1)
            return SRet<bool>(EXIT_FAILURE, false, "Invalid cookie pair");

        std::string key = cookie.substr(0, equalPos);
        std::string value = cookie.substr(equalPos + 1);

        if (key.find_first_of("\0\r\n") != std::string::npos || value.find_first_of("\0\r\n") != std::string::npos)
            return SRet<bool>(EXIT_FAILURE, false, "Invalid cookie pair: control characters detected");

        key = urlDecode(key);
        value = urlDecode(value);

        _cookies[key] = value;
    }

    return SRet<bool>(EXIT_SUCCESS, true);
}

std::string localhostToIp(const std::string &host)
{

    size_t colon_pos = host.find(':');
    std::string hostname = host;
    std::string port = "";
    if (colon_pos != std::string::npos)
    {
        hostname = host.substr(0, colon_pos);
        port = host.substr(colon_pos + 1);
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname.c_str(), NULL, &hints, &res) != 0)
    {
        std::cerr << "Error getting address info for " << hostname << std::endl;
        return "";
    }

    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
    inet_ntop(AF_INET, &ipv4->sin_addr, ip, sizeof(ip));

    freeaddrinfo(res);

    std::string result = std::string(ip);
    if (!port.empty())
    {
        result += ":" + port;
    }

    return result;
}

std::string removeLocationFromPath(std::string path, std::string location)
{
    if (path == "")
        return path;
    if (path.find(location) == 0)
    {
        std::string newPath = path.substr(location.size());
        if (newPath == "")
            return "/";
        if (newPath[0] != '/')
            newPath = "/" + newPath;
        return newPath;
    }
    if (path[0] != '/')
        return "/" + path;
    return path;
}

SRet<std::string> RequestParser::prepareResponse(MimeTypes &mimeTypes)
{
    std::map<std::string, std::string>::iterator it = _headers.find("Host");
    if (it == _headers.end()) // Is it should look like this??
        return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(400));

    std::string host = localhostToIp(trim(it->second));
    Config serverConfig = config.getServerConfig(host);
    if (serverConfig.getPort() == 0)
        return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(400));

    Location loc = serverConfig.getCorrentLocation(_path);

    for (size_t i = 0; i < loc.getAllowMethods().size(); i++)
    {
        if (loc.getAllowMethods()[i] == _method)
            break;
        if (i == loc.getAllowMethods().size() - 1)
            return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(405, loc));
    }

    if (loc.getClientMaxBodySize() != -1 && _body.size() > static_cast<size_t>(loc.getClientMaxBodySize()))
        return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(413, loc));

    if (loc.getReturn().first != -1)
    {
        std::pair<int, std::string> return_ = loc.getReturn();
        std::string response = "HTTP/1.1 " + size_tToString(return_.first) + " " + return_.second + "\r\nLocation: " + return_.second + "\r\n\r\n";
        return SRet<std::string>(EXIT_SUCCESS, response);
    }


    /**
     * this is the part where we check if the request is a CGI request
     * if it is, we run the CGI script and return the response
     * if it is not, we continue with the normal flow
     */
    bool isCGI = false;
    if (loc.getCgiParams().size() > 0 && _path.find_last_of(".") != std::string::npos) {
        std::string requestFileExtension = _path.substr(_path.find_last_of(".") + 1);
        std::map<std::string, std::string> cgi_params = loc.getCgiParams();
        for (std::map<std::string, std::string>::const_iterator it = cgi_params.begin(); it != cgi_params.end(); ++it)
        {
            if (requestFileExtension == it->first)
            {
                isCGI = true;
                break;
            }
        }
    }

    if (isCGI)
    {
        CGIRunner cgiRunner = CGIRunner(loc);
        return cgiRunner.runCGI(_path, _params, _method, _cookies, _body, _headers);
    }
    else
    {

        if (loc.getAutoindex() == "on")
        {

            std::string root = loc.getRoot();
            std::string path = root + _path;
            DIR *dir = opendir(path.c_str());
            if (dir == NULL)
            {

                struct stat buffer;
                if (stat(path.c_str(), &buffer) == 0)
                {
                    if (S_ISREG(buffer.st_mode))
                    {

                        std::ifstream file(path.c_str());
                        if (!file.is_open())
                            return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(404, loc));

                        std::string response;
                        std::string line;
                        while (std::getline(file, line))
                            response += line + "\n";
                        file.close();

                        std::string contentType = "text/html";

                        response = "HTTP/1.1 200 OK\r\nContent-Type: " + contentType + "\r\nContent-Length: " + size_tToString(response.size()) + "\r\n\r\n" + response;
                        return SRet<std::string>(EXIT_SUCCESS, response);
                    }
                    else
                        return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(404, loc));
                }
                else
                    return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(404, loc));
            }
            else
            {

                std::string response = "<html><head><title>Index of " + _path + "</title></head><body><h1>Index of " + _path + "</h1><hr><pre>";
                struct dirent *ent;
                while ((ent = readdir(dir)) != NULL)
                {
                    response += "<a href=\"" + _path + "/" + ent->d_name + "\">" + ent->d_name + "</a>\n";
                }
                closedir(dir);

                response += "</pre><hr></body></html>";
                response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + size_tToString(response.size()) + "\r\n\r\n" + response;
                return SRet<std::string>(EXIT_SUCCESS, response);
            }
        }
        else
        {

            std::string index = loc.getIndex();
            std::string root = loc.getRoot();
            std::string _subpath = removeLocationFromPath(_path, loc.getPath());

            std::string path = root + _subpath;

            bool found = false;
            struct stat buffer;
            if (stat(path.c_str(), &buffer) == 0)
            {
                if (S_ISDIR(buffer.st_mode))
                {
                    if (index == "")
                        return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(404, loc));
                    path += "/" + index;
                    found = true;
                }
            }
            else
            {
                // path = "error-pages/error.html";
                return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(404, loc));

            }
            std::ifstream file(path.c_str());
            if (!file.is_open())
                return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(404, loc));

           // read the file with the given path use stream
            std::ostringstream oss;
            oss << file.rdbuf();
            std::string fileContent = oss.str();
            file.close();

            /**
             * TODO:
             * 1. Check if the file is an image, video, etc. and set the content type accordingly
             */
            std::string extension = path.substr(path.find_last_of(".") + 1);
            std::string contentType = mimeTypes.getMimeType("."+extension);

            // std::string responseLine = found ? "200 OK" : "404 Not Found";
            std::string responseLine = "200 OK";
            (void)found;

            std::string response = "HTTP/1.1 "+responseLine+"\r\nContent-Type: " + contentType + "\r\nContent-Length: " + size_tToString(fileContent.size()) + "\r\n\r\n" + fileContent;
            return SRet<std::string>(EXIT_SUCCESS, response);
        }
    }
}
