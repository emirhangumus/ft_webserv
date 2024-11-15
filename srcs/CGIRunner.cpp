#include "CGIRunner.hpp"
#include "ErrorResponse.hpp"
#include "Utils.hpp"
#include <sys/stat.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <cstring>
#include <sys/wait.h>
#include <cerrno>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <map>

CGIRunner::CGIRunner(Location loc)
{
    _loc = loc;
}

CGIRunner::~CGIRunner()
{
}

std::string extractFileName(const std::string &path)
{
    size_t pos = path.find_last_of("/");
    if (pos == std::string::npos)
        return path;
    return path.substr(pos + 1);
}

std::string get_absolute_path(const std::string &relative_path) {
    char absolute_path[1024];
    if (realpath(relative_path.c_str(), absolute_path) == NULL) {
        std::cerr << "Error resolving absolute path: " << strerror(errno) << std::endl;
        return "";
    }
    return std::string(absolute_path);
}

std::string mapToString(const std::map<std::string, std::string> &params)
{
    std::string result;
    for (std::map<std::string, std::string>::const_iterator it = params.begin(); it != params.end(); ++it)
    {
        result += it->first + "=" + it->second + "&";
    }
    if (!result.empty())
        result.erase(result.size() - 1);
    return result;
}

std::string cookieMapToString(const std::map<std::string, std::string> &cookies)
{
    std::string result;
    for (std::map<std::string, std::string>::const_iterator it = cookies.begin(); it != cookies.end(); ++it)
    {
        result += it->first + "=" + it->second + "; ";
    }
    if (!result.empty())
        result.erase(result.size() - 1);
    return result;
}

std::vector<std::string> pathToRequestURIandScriptName(const std::string &path)
{
    std::vector<std::string> result;
    std::string requestURI = path.substr(0, path.find_last_of("/") + 1);
    std::string scriptName = path.substr(path.find_last_of("/") + 1);
    result.push_back(requestURI);
    result.push_back(scriptName);
    return result;
}
SRet<std::string> CGIRunner::runCGI(const std::string &_path, const std::map<std::string, std::string> &_params, 
                                    const std::string &_method, const std::map<std::string, std::string> &_cookies, 
                                    const std::string &_body, const std::map<std::string, std::string> &_headers)
{
    std::string extension = _path.substr(_path.find_last_of(".") + 1);
    const std::map<std::string, std::string>& cgi_params = _loc.getCgiParams();
    std::string cgiRunnerPath;

    for (std::map<std::string, std::string>::const_iterator param_it = cgi_params.begin(); 
         param_it != cgi_params.end(); ++param_it) {
        if (param_it->first == extension) {
            cgiRunnerPath = param_it->second;
        }
    }

    std::string file = _loc.getRoot() + "/" + extractFileName(_path);
    std::string absPath = get_absolute_path(file);
    if (absPath.empty()) {
        return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(404, _loc));
    }

    // Ensure uploads directory exists
    std::string uploadsDir = _loc.getRoot() + "/tmp";
    mkdir(uploadsDir.c_str(), 0755);

    std::vector<std::string> requestURIandScriptName = pathToRequestURIandScriptName(_path);

    // Prepare environment variables
    std::string contentLength = "CONTENT_LENGTH=" + size_tToString(_body.length());
    std::string contentType = "CONTENT_TYPE=";
    if (_headers.find("Content-Type") != _headers.end()) {
        contentType += _headers.find("Content-Type")->second;
    }

    // Extract boundary if it exists
    // size_t boundaryPos = contentType.find("boundary=");
    // if (boundaryPos != std::string::npos) {
    //     contentType += contentType.substr(boundaryPos + 9);
    // }

    std::string _REQUEST_URI = "REQUEST_URI=" + requestURIandScriptName[0];
    std::string _SCRIPT_FILENAME = "SCRIPT_FILENAME=" + absPath;
    std::string _QUERY_STRING = "QUERY_STRING=" + mapToString(_params);
    std::string _METHOD = "REQUEST_METHOD=" + _method;
    std::string _HTTP_COOKIE = "HTTP_COOKIE=" + cookieMapToString(_cookies);
    std::string _PATH_INFO = "PATH_INFO=" + _path;

    char *argv[] = {const_cast<char *>(cgiRunnerPath.c_str()), const_cast<char *>(file.c_str()), NULL};
    
    char *envp[] = {
        const_cast<char *>("AUTH_TYPE="),
        const_cast<char *>(contentLength.c_str()),
        const_cast<char *>(contentType.c_str()),
        const_cast<char *>("REDIRECT_STATUS=200"),
        const_cast<char *>("GATEWAY_INTERFACE=CGI/1.1"),
        const_cast<char *>(_QUERY_STRING.c_str()),
        const_cast<char *>("REMOTE_ADDR="),
        const_cast<char *>("REMOTE_IDENT="),
        const_cast<char *>("REMOTE_USER="),
        const_cast<char *>(_METHOD.c_str()),
        const_cast<char *>(_REQUEST_URI.c_str()),
        const_cast<char *>(_SCRIPT_FILENAME.c_str()),
        const_cast<char *>("SERVER_NAME=localhost"),
        const_cast<char *>("SERVER_PORT=8080"),
        const_cast<char *>("SERVER_PROTOCOL=HTTP/1.1"),
        const_cast<char *>("SERVER_SOFTWARE=webserv"),
        const_cast<char *>(_HTTP_COOKIE.c_str()),
        const_cast<char *>(_PATH_INFO.c_str()),
        NULL
    };

    int pipefd[2];
    int bodyPipe[2];
    if (pipe(pipefd) == -1 || pipe(bodyPipe) == -1) {
        return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(500, _loc));
    }

    pid_t pid = fork();
    if (pid == -1) {
        return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(500, _loc));
    }

    if (pid == 0) {  // Child process
        close(pipefd[0]);
        close(bodyPipe[1]);

        dup2(pipefd[1], STDOUT_FILENO);
        dup2(bodyPipe[0], STDIN_FILENO);
        close(pipefd[1]);
        close(bodyPipe[0]);

        if (execve(cgiRunnerPath.c_str(), argv, envp) == -1) {
            std::cerr << "CGI execution failed: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
    } else {  // Parent process
        close(pipefd[1]);
        close(bodyPipe[0]);

        if (!_body.empty()) {
            if (write(bodyPipe[1], _body.c_str(), _body.size()) == -1) {
                std::cerr << "Failed to write to CGI input: " << strerror(errno) << std::endl;
            }
        }
        close(bodyPipe[1]);

        // Read the CGI output
        std::string output;
        char buffer[4096];
        ssize_t bytesRead;
        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;
        }
        close(pipefd[0]);

        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            bool hasHeaders = true;
            // check output has `\r\n\r\n` or not
            if (output.find("\r\n\r\n") == std::string::npos) { 
                hasHeaders = false;
            }

            std::string headers = "";
            if (!hasHeaders) {
                // Check if the CGI script has already set the Content-Type and Content-Length headers
                if (output.find("Content-Type:") == std::string::npos) {
                    headers += "Content-Type: text/html\r\n";
                }
                if (output.find("Content-Length:") == std::string::npos) {
                    headers += "Content-Length: " + size_tToString(output.size()) + "\r\n";
                }
                headers += "\r\n";
            } else {
                size_t pos = output.find("\r\n\r\n");
                headers = output.substr(0, pos);
                output = output.substr(pos + 4);

                headers += "\r\n";

                // Check if the CGI script has already set the Content-Length header
                if (headers.find("Content-Length:") == std::string::npos) {
                    headers += "Content-Length: " + size_tToString(output.size()) + "\r\n";
                }
            }
            headers = "HTTP/1.1 200 OK\r\n" + headers;
            output = headers + "\r\n" + output;

            return SRet<std::string>(0, output); 
        }
        return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(500, _loc));
    }

    return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(500, _loc));
}
