#ifndef CACHEMANAGER_HPP
#define CACHEMANAGER_HPP

#include <map>
#include <string>

class CacheManager
{
public:
    CacheManager();
    ~CacheManager();

    void addCache(const std::string &key, const std::string &value);
    std::string getCache(const std::string &key);
    void clearCache();
private:
    std::map<std::string, std::string> cache;
    unsigned long long getCurrentTime() const;
    unsigned long long _lastInvalidation;
};

#endif