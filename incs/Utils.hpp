#ifndef UTILS_HPP
#define UTILS_HPP

#include <utility>
#include <string>
#include <stdexcept>
#include <vector>

template <typename T>
class SRet // Stands for: Safe Return
{
public:
    int status;
    T data;
    std::string err;

    SRet(int f, T s, const std::string &e) : status(f), data(s), err(e) {}
    SRet(int f, T s) : status(f), data(s) {}
};

std::string trim(const std::string &str);
std::vector<std::string> split(const std::string &str, char delim);

#endif