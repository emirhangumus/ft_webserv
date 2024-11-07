#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "Location.hpp"
#include <string>
#include <map>
#include <vector>
#include <algorithm>

struct LocationComparator {
    bool operator()(const std::string& lhs, const std::string& rhs) const {
        // Count '/' characters in lhs and rhs
        int lhsSlashCount = std::count(lhs.begin(), lhs.end(), '/');
        int rhsSlashCount = std::count(rhs.begin(), rhs.end(), '/');
        
        // First, compare by '/' count in descending order
        if (lhsSlashCount != rhsSlashCount) {
            return lhsSlashCount > rhsSlashCount;
        }

        // Then look for the length of the string
        if (lhs.size() != rhs.size()) {
            return lhs.size() > rhs.size();
        }
        
        // If '/' counts are the same, compare alphabetically in ascending order
        return lhs < rhs;
    }
};

class Config
{
public:
    Config();
    ~Config();

    void fillConfig(std::string listen, std::vector<std::string> server_names, Location main_location, std::map<std::string, Location, LocationComparator> locations);
    std::pair<std::string, unsigned int> getListen() const { return listen; }
    unsigned int getPort() const { return listen.second; }
    std::vector<std::string> getServerNames() const { return server_names; }
    std::map<std::string, Location, LocationComparator> getLocations() const { return locations; }
    Location getMainLocation() const { return main_location; }
    Location getCorrentLocation(std::string path);

private:
    std::pair<std::string, unsigned int> listen;
    std::vector<std::string> server_names;
    std::map<std::string, Location, LocationComparator> locations;
    Location main_location;
};

#endif