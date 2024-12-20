#include "Location.hpp"
#include "Utils.hpp"
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <iostream>

Location::Location()
{
    this->path = "";
    this->root = "";
    this->index = "";
    this->autoindex = "off";
    this->client_max_body_size = -1;
    this->allow_methods = std::vector<std::string>();
    this->cgi_params = std::map<std::string, std::string>();
    this->error_pages = std::map<int, std::string>();
    this->return_ = std::pair<int, std::string>(-1, "");
}

Location::~Location()
{
}

void Location::setPath(const std::string &path)
{
    this->path = path;
}

void Location::setRoot(const std::string &root)
{
    this->root = root;
}

void Location::setIndex(const std::string &index)
{
    this->index = index;
}

void Location::setAutoindex(const std::string &autoindex)
{
    this->autoindex = autoindex;
}

void Location::setClientMaxBodySize(long long client_max_body_size)
{
    this->client_max_body_size = client_max_body_size;
}

void Location::setAllowMethods(const std::vector<std::string> allow_methods)
{
    for (std::vector<std::string>::const_iterator it = allow_methods.begin(); it != allow_methods.end(); ++it)
    {
        if (!isMethod(*it))
            throw std::runtime_error("Error: invalid config file: allow_methods directive must have valid methods");
    }
    
    this->allow_methods = allow_methods;
}

void Location::setCgiParams(const std::map<std::string, std::string> &cgi_params)
{
    this->cgi_params = cgi_params;
}

void Location::setErrorPages(const std::map<int, std::string>& error_pages)
{
    this->error_pages = error_pages;
}

void Location::addErrorPage(int code, const std::string &error_page)
{
    this->error_pages[code] = error_page;
}

void Location::setReturn(const std::pair<int, std::string>& return_)
{
    if ((return_.first < 100 || return_.first > 599) && return_.first != -1)
        throw std::runtime_error("Error: invalid config file: return code must be between 100 and 599");
    this->return_ = return_;
} 

void Location::printLocation() const
{
    std::cout << "―― Location: " << this->path << std::endl;
    std::cout << "―― Root: " << this->root << std::endl;
    std::cout << "―― Index: " << this->index << std::endl;
    std::cout << "―― Autoindex: " << this->autoindex << std::endl;
    std::cout << "―― Client Max Body Size: " << this->client_max_body_size << std::endl;
    std::cout << "―― Allow Methods: ";
    for (std::vector<std::string>::const_iterator it = this->allow_methods.begin(); it != this->allow_methods.end(); ++it)
    {
        std::cout << "―― " << *it << " ";
    }
    std::cout << std::endl;
    std::cout << "―― Return: " << this->return_.first << " " << this->return_.second << std::endl;

    std::cout << "―― Error Pages: " << std::endl;
    for (std::map<int, std::string>::const_iterator it = this->error_pages.begin(); it != this->error_pages.end(); ++it)
    {
        std::cout << "―― " << it->first << " = " << it->second << std::endl;
    }


    std::cout << "―― Cgi Params: " << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = this->cgi_params.begin(); it != this->cgi_params.end(); ++it)
    {
        std::cout << "―― " << it->first << " = " << it->second << std::endl;
    }
}

