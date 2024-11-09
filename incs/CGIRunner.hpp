#ifndef CGIRUNNER_HPP
#define CGIRUNNER_HPP

#include <string>
#include <vector>
#include <map>
#include "Utils.hpp"
#include "Location.hpp"

class CGIRunner
{
public:
    CGIRunner(Location loc);
    ~CGIRunner();

    SRet<std::string> runCGI(const std::string &_path, const std::map<std::string, std::string> &_params, const std::string &_method, const std::map<std::string, std::string> &_cookies, const std::string &_body, const std::map<std::string, std::string> &_headers);
private:
    Location _loc;
};

#endif