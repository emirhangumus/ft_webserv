#include "MimeTypes.hpp"

MimeTypes::MimeTypes()
{
    _mimes[".html"] = "text/html";
    _mimes[".htm"] = "text/html";
    _mimes[".css"] = "text/css";
    _mimes[".ico"] = "image/x-icon";
    _mimes[".avi"] = "video/x-msvideo";
    _mimes[".bmp"] = "image/bmp";
    _mimes[".doc"] = "application/msword";
    _mimes[".gif"] = "image/gif";
    _mimes[".gz"] = "application/x-gzip";
    _mimes[".ico"] = "image/x-icon";
    _mimes[".jpg"] = "image/jpeg";
    _mimes[".jpeg"] = "image/jpeg";
    _mimes[".png"] = "image/png";
    _mimes[".txt"] = "text/plain";
    _mimes[".mp3"] = "audio/mp3";
    _mimes[".pdf"] = "application/pdf";
    _mimes["default"] = "text/html";
}

std::string MimeTypes::getMimeType(std::string extension)
{
    if (_mimes.count(extension))
        return (_mimes[extension]);
     return (_mimes["default"]);
}