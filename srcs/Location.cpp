#include "Location.hpp"
#include <iostream>

Location::Location()
{
    // std::cout << "Location Constructor called" << std::endl;

    this->path = "";
    this->root = "";
    this->index = "";
    this->autoindex = "";
    this->client_max_body_size = "";
    this->allow_methods = std::vector<std::string>();
    this->limit_except = "";
    this->cgi_path = "";
    this->error_page = "";
    this->return_ = "";
}

Location::~Location()
{
    // std::cout << "Location Destructor called" << std::endl;
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

void Location::setClientMaxBodySize(const std::string &client_max_body_size)
{
    this->client_max_body_size = client_max_body_size;
}

void Location::setAllowMethods(const std::vector<std::string> allow_methods)
{
    this->allow_methods = allow_methods;
}

void Location::setLimitExcept(const std::string &limit_except)
{
    this->limit_except = limit_except;
}

void Location::setCgiPath(const std::string &cgi_path)
{
    this->cgi_path = cgi_path;
}

void Location::setErrorPage(const std::string &error_page)
{
    this->error_page = error_page;
}

void Location::setReturn(const std::string &return_)
{
    this->return_ = return_;
}

void Location::setTryFiles(const std::string &try_files)
{
    this->try_files = try_files;
}