#ifndef MIMETYPES_HPP
#define MIMETYPES_HPP

#include <string>
#include <map>

class MimeTypes
{
public:
    MimeTypes();
    std::string getMimeType(std::string extension);

private:
    std::map<std::string, std::string> _mimes;
};

#endif