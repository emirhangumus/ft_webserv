#ifndef ERRORRESPONSE_HPP
#define ERRORRESPONSE_HPP

#include <string>

class ErrorResponse
{
public:
    static std::string getErrorResponse(int code);
};

#endif