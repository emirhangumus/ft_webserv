#include "Location.hpp"
#include <iostream>

#include <string>
#include <vector>
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

void Location::printLocation() const
{
    std::cout << "Location: " << this->path << std::endl;
    std::cout << "Root: " << this->root << std::endl;
    std::cout << "Index: " << this->index << std::endl;
    std::cout << "Autoindex: " << this->autoindex << std::endl;
    std::cout << "Client Max Body Size: " << this->client_max_body_size << std::endl;
    std::cout << "Allow Methods: ";
    for (std::vector<std::string>::const_iterator it = this->allow_methods.begin(); it != this->allow_methods.end(); ++it)
    {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    std::cout << "Cgi Path: " << this->cgi_path << std::endl;
    std::cout << "Error Page: " << this->error_page << std::endl;
    std::cout << "Return: " << this->return_ << std::endl;
    std::cout << "Try Files: " << this->try_files << std::endl;
}

