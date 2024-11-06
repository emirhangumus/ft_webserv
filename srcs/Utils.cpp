#include "Utils.hpp"

std::string trim(const std::string &str)
{
    size_t start = 0;
    size_t end = str.size();

    while (start < end && std::isspace(str[start]))
    {
        start++;
    }

    while (end > start && std::isspace(str[end - 1]))
    {
        end--;
    }

    return str.substr(start, end - start);
}

std::vector<std::string> split(const std::string &str, char delim) {
    std::vector<std::string> tokens;
    std::string token;

    for (size_t i = 0; i < str.length(); ++i) {
        char c = str[i];
        if (c == delim) {
            tokens.push_back(token);
            token.clear();
        } else {
            token += c;
        }
    }
    
    tokens.push_back(token); // Add the last token
    return tokens;
}

unsigned int stringtoui(const std::string &str)
{
    unsigned int result = 0;
    int sign = 1;
    size_t i = 0;

    if (str[0] == '-')
    {
        sign = -1;
        i++;
    }

    for (; i < str.size(); i++)
    {
        result = result * 10 + str[i] - '0';
    }

    return result * sign;
}

// void bzero(void *s, size_t n)
// {
//     char *ptr = (char *)s;
//     while (n--)
//         *ptr++ = 0;
// }