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
        result += it->first + "=" + it->second + ";";
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

SRet<std::string> CGIRunner::runCGI(const std::string &_path, const std::map<std::string, std::string> &_params, const std::string &_method, const std::map<std::string, std::string> &_cookies, const std::string &_body)
{
    // find file extension in _path
    std::string extension = _path.substr(_path.find_last_of(".") + 1);
    const std::map<std::string, std::string>& cgi_params = _loc.getCgiParams();
    std::string cgiRunnerPath;
    for (std::map<std::string, std::string>::const_iterator param_it = cgi_params.begin(); param_it != cgi_params.end(); ++param_it) 
        if (param_it->first == extension)
            cgiRunnerPath = param_it->second;

    std::string file = _loc.getRoot() + "/" + extractFileName(_path);

    std::string absPath = get_absolute_path(file);
    if (absPath.empty())
        return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(404));
    
    std::vector<std::string> requestURIandScriptName = pathToRequestURIandScriptName(_path);
    std::string _REQUEST_URI = "REQUEST_URI=" + requestURIandScriptName[0];
    // std::string _SCRIPT_NAME = "SCRIPT_NAME=" + requestURIandScriptName[1];
    std::string _SCRIPT_FILENAME = "SCRIPT_FILENAME=" + absPath;
    std::string _QUERY_STRING = "QUERY_STRING=" + mapToString(_params) + _body;
    std::string _METHOD = "REQUEST_METHOD=" + _method;
    std::string _HTTP_COOKIE = "HTTP_COOKIE=" + cookieMapToString(_cookies);
    std::string _PATH_INFO = "PATH_INFO=" + _path;

    std::cout << "_QUERY_STRING: " << _QUERY_STRING << std::endl;

    char *argv[] = {const_cast<char *>(cgiRunnerPath.c_str()), const_cast<char *>(file.c_str()), NULL};
    char *envp[] = {
        const_cast<char *>("AUTH_TYPE="),
        const_cast<char *>("CONTENT_LENGTH=0"),
        const_cast<char *>("CONTENT_TYPE=application/x-www-form-urlencoded"),
        const_cast<char *>("REDIRECT_STATUS=200"),
        const_cast<char *>("GATEWAY_INTERFACE=CGI/1.1"),
        const_cast<char *>(_QUERY_STRING.c_str()),  // The query string
        const_cast<char *>("REMOTE_ADDR="),
        const_cast<char *>("REMOTE_IDENT="),
        const_cast<char *>("REMOTE_USER="),
        const_cast<char *>(_METHOD.c_str()),  // The method of the request
        const_cast<char *>(_REQUEST_URI.c_str()),  // The URI of the request
        // const_cast<char *>(_SCRIPT_NAME.c_str()),  // The name of the script
        const_cast<char *>(_SCRIPT_FILENAME.c_str()),  // The path of the script
        const_cast<char *>("SERVER_NAME=localhost"),
        const_cast<char *>("SERVER_PORT=8080"),
        const_cast<char *>("SERVER_PROTOCOL=HTTP/1.1"),
        const_cast<char *>("SERVER_SOFTWARE=webserv"),
        const_cast<char *>(_HTTP_COOKIE.c_str()),  // The cookies
        const_cast<char *>(_PATH_INFO.c_str()),  // The path info
        NULL
    };
    
    int pipefd[2];
    int bodyPipe[2];
    if (pipe(pipefd) == -1 || pipe(bodyPipe) == -1)
        return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(500));

    pid_t pid = fork();
    if (pid == -1)
        return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(500));

    if (pid == 0)
    {
        close(pipefd[0]);
        close(bodyPipe[1]);

        dup2(pipefd[1], STDOUT_FILENO);
        dup2(bodyPipe[0], STDIN_FILENO);
        close(pipefd[1]);
        close(bodyPipe[0]);

        if (execve(cgiRunnerPath.c_str(), argv, envp) == -1)
            exit(EXIT_FAILURE);  // Exit if execve fails
    }
    else
    {
        close(pipefd[1]);
        close(bodyPipe[0]);

        if (!_body.empty())
        {
            write(bodyPipe[1], _body.c_str(), _body.size());
        }
        close(bodyPipe[1]);

        std::string output;
        char buffer[4096];
        int bytesRead;
        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[bytesRead] = '\0';
            output += buffer;
        }
        close(pipefd[0]);

        int status;
        waitpid(pid, &status, 0);
        std::cout << "Output: " << output << std::endl;
        std::string responseLine = "HTTP/1.1 200 OK\r\n";
        std::string _output = responseLine + output;
        
        if (WIFEXITED(status))
            return SRet<std::string>(WEXITSTATUS(status), _output, "");
    }
    return SRet<std::string>(EXIT_FAILURE, "", ErrorResponse::getErrorResponse(500));
}