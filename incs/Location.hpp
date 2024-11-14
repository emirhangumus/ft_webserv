#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <vector>
#include <map>
#include "Utils.hpp"

class Location
{
public:
    Location();
    ~Location();

    void setPath(const std::string &path);
    void setRoot(const std::string &root);
    void setIndex(const std::string &index);
    void setAutoindex(const std::string &autoindex);
    void setClientMaxBodySize(long long client_max_body_size);
    void setAllowMethods(const std::vector<std::string> allow_methods);
    void setCgiParams(const std::map<std::string, std::string> &cgi_params);
    void setReturn(const std::pair<int, std::string>& return_);
    void setErrorPages(const std::map<int, std::string>& error_pages);

    void addErrorPage(int code, const std::string &error_page);

    std::string getPath() const { return path; }
    std::string getRoot() const { return root; }
    std::string getIndex() const { return index; }
    std::string getAutoindex() const { return autoindex; }
    long long getClientMaxBodySize() const { return client_max_body_size; }
    std::vector<std::string> getAllowMethods() const { return allow_methods; }
    std::map<std::string, std::string> getCgiParams() const { return cgi_params; }
    std::pair<int, std::string> getReturn() const { return return_; }
    std::map<int, std::string> getErrorPages() const { return error_pages; }

    void printLocation() const;

private:
    std::string path;
    std::string root;
    std::string index;
    std::string autoindex;
    std::pair<int, std::string> return_;
    long long client_max_body_size;
    std::vector<std::string> allow_methods;
    std::map<std::string, std::string> cgi_params;
    std::map<int, std::string> error_pages;
};

#endif