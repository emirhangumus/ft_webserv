#include "CacheManager.hpp"
#include <ctime>
#include <iostream>

CacheManager::CacheManager()
{
    _lastInvalidation = getCurrentTime();
}

unsigned long long CacheManager::getCurrentTime() const {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    
    return static_cast<unsigned long long>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000;
}

CacheManager::~CacheManager()
{
    clearCache();
}

void CacheManager::addCache(const std::string &key, const std::string &value)
{
    cache[key] = value;
}

std::string CacheManager::getCache(const std::string &key)
{
    if (getCurrentTime() - _lastInvalidation > 1000)
    {
        clearCache();
        _lastInvalidation = getCurrentTime();
    } else {
        std::map<std::string, std::string>::const_iterator it = cache.find(key);
        if (it != cache.end())
            return it->second;
    }
    return "";
}

void CacheManager::clearCache()
{
    std::cout << "Clearing cache" << std::endl;
    cache.clear();
}
