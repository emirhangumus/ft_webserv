#ifndef ERRORRESPONSE_HPP
#define ERRORRESPONSE_HPP

#include <string>
#include "Location.hpp"

class ErrorResponse
{
public:
    static std::string getErrorResponse(int code);
    static std::string getErrorResponse(int code, Location &loc);
};

#endif