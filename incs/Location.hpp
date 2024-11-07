#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <vector>
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
    void setClientMaxBodySize(const std::string &client_max_body_size);
    void setAllowMethods(const std::vector<std::string> allow_methods);
    void setLimitExcept(const std::string &limit_except);
    void setCgiPath(const std::string &cgi_path);
    void setErrorPage(const std::string &error_page);
    void setReturn(const std::string &return_);
    void setTryFiles(const std::string &try_files);

    void setPathDeepLevel(unsigned int path_deep_level) { _path_deep_level = path_deep_level; }

    std::string getPath() const { return path; }
    std::string getRoot() const { return root; }
    std::string getIndex() const { return index; }
    std::string getAutoindex() const { return autoindex; }
    std::string getClientMaxBodySize() const { return client_max_body_size; }
    std::vector<std::string> getAllowMethods() const { return allow_methods; }
    std::string getLimitExcept() const { return limit_except; }
    std::string getCgiPath() const { return cgi_path; }
    std::string getErrorPage() const { return error_page; }
    std::string getReturn() const { return return_; }
    std::string getTryFiles() const { return try_files; }

    void printLocation() const;

private:
    std::string path;
    std::string root;
    std::string index;
    std::string autoindex;
    std::string return_;
    std::string try_files;
    std::string client_max_body_size;
    std::vector<std::string> allow_methods;
    std::string limit_except;
    std::string cgi_path;
    std::string error_page;

    unsigned int _path_deep_level;
};

#endif