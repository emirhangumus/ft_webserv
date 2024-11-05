#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "Location.hpp"
#include <string>
#include <map>
#include <vector>

class Config
{
public:
    Config();
    ~Config();

    void fillConfig(std::string listen, std::vector<std::string> server_names, Location main_location, std::map<std::string, Location> locations);
    std::pair<std::string, int> getListen() const { return listen; }
    std::vector<std::string> getServerNames() const { return server_names; }
    Location getMainLocation() const { return main_location; }
    std::map<std::string, Location> getLocations() const { return locations; }

private:
    std::pair<std::string, int> listen;
    std::vector<std::string> server_names;
    Location main_location;
    std::map<std::string, Location> locations;
};

#endif