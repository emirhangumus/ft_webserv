#include "Utils.hpp"
#include <sstream>
#include <map>
#include <limits>

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
    tokens.reserve(str.length() / 2);  // Reserve space for efficiency

    for (size_t i = 0; i < str.length(); ++i) {
        char c = str[i];
        if (c == delim) {
            if (!token.empty()) {  // Only add non-empty tokens
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }

    // Handle the last token, if any
    if (!token.empty()) {
        tokens.push_back(token);
    }

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

unsigned int countThis(std::string str, char c)
{
    unsigned int count = 0;
    for (size_t i = 0; i < str.size(); i++)
    {
        if (str[i] == c)
            count++;
    }
    return count;
}

std::string size_tToString(size_t value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

long long convertSizeToBytes(const std::string& sizeStr) {
    std::map<std::string, long long> unitMap;
    unitMap["KB"] = 1024LL;
    unitMap["MB"] = 1024LL * 1024;
    unitMap["GB"] = 1024LL * 1024 * 1024;
    unitMap["TB"] = 1024LL * 1024 * 1024 * 1024;

    size_t i = 0;
    while (i < sizeStr.size() && (isdigit(sizeStr[i]) || sizeStr[i] == '.')) ++i;
    
    if (i == 0 || i == sizeStr.size()) {
        return -1;
    }

    std::istringstream numStream(sizeStr.substr(0, i));
    double number;
    numStream >> number;

    std::string unit = sizeStr.substr(i);
    for (size_t j = 0; j < unit.size(); ++j) {
        unit[j] = toupper(unit[j]);
    }

    std::map<std::string, long long>::iterator it = unitMap.find(unit);
    if (it == unitMap.end()) {
        return -1;
    }

    return static_cast<long long>(number * it->second);
}

bool compareSizeTandLongLong(size_t sizeVal, long long longVal) {
    // Check if longVal is negative
    if (longVal < 0) {
        return false;
    }

    // Convert longVal to size_t if within the range of size_t
    if (static_cast<unsigned long long>(longVal) > std::numeric_limits<size_t>::max()) {
        return false;
    }

    return sizeVal == static_cast<size_t>(longVal);
}