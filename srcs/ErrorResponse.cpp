#include "Utils.hpp"
#include "ErrorResponse.hpp"
#include "Location.hpp"
#include <map>
#include <iostream>
#include <fstream>

std::string ErrorResponse::getErrorResponse(int code, Location &loc)
{
    if (loc.getErrorPages().size() > 0 && loc.getErrorPages().find(code) != loc.getErrorPages().end())
    {
        try {
            std::string error_file_path = loc.getRoot() + loc.getErrorPages()[code];
            std::ifstream file(error_file_path.c_str());
            if (!file.is_open())
                throw std::runtime_error("Error: could not open file: " + error_file_path);
            std::string response((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
            std::string contentType = "text/html";
            std::map<std::string, std::string> headers;
            headers["Content-Type"] = contentType;
            headers["Content-Length"] = size_tToString(response.size());
            headers["Connection"] = "close";
            std::string responseStr = "HTTP/1.1 " + getTitleAndBody(code).first + "\r\n";
            for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); it++)
            {
                responseStr += it->first + ": " + it->second + "\r\n";
            }
            responseStr += "\r\n" + response;
            return responseStr;
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << std::endl;
        }
        return getErrorResponse(code);
    }
    else
    {
        return getErrorResponse(code);
    }
}

std::string ErrorResponse::getErrorResponse(int code)
{
    std::pair<std::string, std::string> titleAndBody = getTitleAndBody(code);
    std::map<std::string, std::string> headers;
    std::string title = titleAndBody.first;
    std::string body = titleAndBody.second;
    std::string totalBody = "<html>\r\n<head><title>" + title + "</title></head>\r\n<body>\r\n<h1>" + title + "</h1>\r\n<p>" + body + "</p>\r\n</body>\r\n</html>";
    headers["Content-Type"] = "text/html";
    headers["Content-Length"] = size_tToString(totalBody.size());
    headers["Connection"] = "close";
    std::string response = "HTTP/1.1 " + title + "\r\n";
    for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); it++)
    {
        response += it->first + ": " + it->second + "\r\n";
    }
    response += "\r\n" + totalBody;

    return response;
}

std::pair<std::string, std::string> ErrorResponse::getTitleAndBody(int code)
{
    std::string title;
    std::string body;
    if (code >= 400 && code <= 599)
    {
        switch (code)
        {
        case 400:
            title = "400 Bad Request";
            body = "Your browser sent a request that this server could not understand.";
            break;
        case 401:
            title = "401 Unauthorized";
            body = "This server could not verify that you are authorized to access the document requested.";
            break;
        case 402:
            title = "402 Payment Required";
            body = "Payment is required to access this resource.";
            break;
        case 403:
            title = "403 Forbidden";
            body = "You don't have permission to access this resource.";
            break;
        case 404:
            title = "404 Not Found";
            body = "The requested resource was not found on this server.";
            break;
        case 405:
            title = "405 Method Not Allowed";
            body = "The method is not allowed for the requested URL.";
            break;
        case 406:
            title = "406 Not Acceptable";
            body = "An appropriate representation of the requested resource could not be found on this server.";
            break;
        case 407:
            title = "407 Proxy Authentication Required";
            body = "This server could not verify that you are authorized to access the document requested.";
            break;
        case 408:
            title = "408 Request Timeout";
            body = "The server closed the network connection because the browser didn't finish the request within the specified time.";
            break;
        case 409:
            title = "409 Conflict";
            body = "The request could not be completed due to a conflict with the current state of the resource.";
            break;
        case 410:
            title = "410 Gone";
            body = "The requested resource is no longer available on this server and there is no forwarding address.";
            break;
        case 411:
            title = "411 Length Required";
            body = "A Content-Length header is required for this request.";
            break;
        case 412:
            title = "412 Precondition Failed";
            body = "One or more conditions given in the request header fields evaluated to false when tested on the server.";
            break;
        case 413:
            title = "413 Payload Too Large";
            body = "The request is larger than the server is willing or able to process.";
            break;
        case 414:
            title = "414 URI Too Long";
            body = "The URI provided was too long for the server to process.";
            break;
        case 415:
            title = "415 Unsupported Media Type";
            body = "The server does not support the media type transmitted in the request.";
            break;
        case 416:
            title = "416 Range Not Satisfiable";
            body = "The requested range is not satisfiable.";
            break;
        case 417:
            title = "417 Expectation Failed";
            body = "The server cannot meet the requirements of the Expect request-header field.";
            break;
        case 418:
            title = "418 I'm a teapot";
            body = "This server is a teapot, not a coffee machine.";
            break;
        case 421:
            title = "421 Misdirected Request";
            body = "The request was directed at a server that is not able to produce a response.";
            break;
        case 422:
            title = "422 Unprocessable Entity";
            body = "The server understands the content type of the request entity, and the syntax of the request entity is correct, but it was unable to process the contained instructions.";
            break;
        case 423:
            title = "423 Locked";
            body = "The resource that is being accessed is locked.";
            break;
        case 424:
            title = "424 Failed Dependency";
            body = "The request failed due to a failure of a previous request.";
            break;
        case 425:
            title = "425 Too Early";
            body = "The server is unwilling to risk processing a request that might be replayed.";
            break;
        case 426:
            title = "426 Upgrade Required";
            body = "The server refuses to perform the request using the current protocol but might be willing to do so after the client upgrades to a different protocol.";
            break;
        case 428:
            title = "428 Precondition Required";
            body = "The server requires the request to be conditional.";
            break;
        case 429:
            title = "429 Too Many Requests";
            body = "The user has sent too many requests in a given amount of time.";
            break;
        case 431:
            title = "431 Request Header Fields Too Large";
            body = "The server is unwilling to process the request because its header fields are too large.";
            break;
        case 451:
            title = "451 Unavailable For Legal Reasons";
            body = "The server is denying access to the resource as a consequence of a legal demand.";
            break;
        case 500:
            title = "500 Internal Server Error";
            body = "The server encountered an unexpected condition that prevented it from fulfilling the request.";
            break;
        case 501:
            title = "501 Not Implemented";
            body = "The server does not support the functionality required to fulfill the request.";
            break;
        case 502:
            title = "502 Bad Gateway";
            body = "The server, while acting as a gateway or proxy, received an invalid response from the upstream server it accessed in attempting to fulfill the request.";
            break;
        case 503:
            title = "503 Service Unavailable";
            body = "The server is currently unable to handle the request due to a temporary overloading or maintenance of the server.";
            break;
        case 504:
            title = "504 Gateway Timeout";
            body = "The server, while acting as a gateway or proxy, did not receive a timely response from the upstream server specified by the URI or some other auxiliary server it needed to access in order to complete the request.";
            break;
        case 505:
            title = "505 HTTP Version Not Supported";
            body = "The server does not support, or refuses to support, the major version of HTTP that was used in the request message.";
            break;
        case 506:
            title = "506 Variant Also Negotiates";
            body = "Transparent content negotiation for the request results in a circular reference.";
            break;
        case 507:
            title = "507 Insufficient Storage";
            body = "The server is unable to store the representation needed to complete the request.";
            break;
        case 508:
            title = "508 Loop Detected";
            body = "The server detected an infinite loop while processing the request.";
            break;
        case 510:
            title = "510 Not Extended";
            body = "Further extensions to the request are required for the server to fulfill it.";
            break;
        case 511:
            title = "511 Network Authentication Required";
            body = "The client needs to authenticate to gain network access.";
            break;
        default:
            title = "500 Internal Server Error";
            body = "The server encountered an unexpected condition that prevented it from fulfilling the request.";
            break;
        }
    }
    return std::make_pair(title, body);
}