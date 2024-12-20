#include "Config.hpp"
#include "Utils.hpp"
#include <iostream>
#include <algorithm>

Config::Config()
{
}

void Config::fillConfig(std::string listen, std::vector<std::string> server_names, Location main_location, std::map<std::string, Location, LocationComparator> locations)
{
    this->server_names = server_names;
    this->locations = locations;
    this->main_location = main_location;
    std::string delimiter = ":";
    size_t pos = listen.find(delimiter);
    this->listen = std::make_pair(listen.substr(0, pos), stringtoui(listen.substr(pos + 1)));

    /**
     * This is for the case when the location block does not have a specific field set.
     * In this case, the location block should inherit the value from the main location block.
     */
    std::map<std::string, Location, LocationComparator>::iterator it;
    for (it = locations.begin(); it != locations.end(); it++) {
        locations[it->first].setAutoindex(locations[it->first].getAutoindex() == "" ? main_location.getAutoindex() : locations[it->first].getAutoindex());
        locations[it->first].setClientMaxBodySize(locations[it->first].getClientMaxBodySize() == -1 ? main_location.getClientMaxBodySize() : locations[it->first].getClientMaxBodySize());
        locations[it->first].setCgiParams(locations[it->first].getCgiParams().empty() ? main_location.getCgiParams() : locations[it->first].getCgiParams());
        locations[it->first].setErrorPages(locations[it->first].getErrorPages().empty() ? main_location.getErrorPages() : locations[it->first].getErrorPages());
        locations[it->first].setIndex(locations[it->first].getIndex() == "" ? main_location.getIndex() : locations[it->first].getIndex());
        locations[it->first].setReturn(locations[it->first].getReturn().first == -1 ? main_location.getReturn() : locations[it->first].getReturn());
        locations[it->first].setRoot(locations[it->first].getRoot() == "" ? main_location.getRoot() : locations[it->first].getRoot());

        std::vector<std::string> allowMethods = locations[it->first].getAllowMethods();
        if (allowMethods.empty()) {
            locations[it->first].setAllowMethods(main_location.getAllowMethods());
        }
    }   

    /**
     * default values for the main location block
     */
    main_location.setAutoindex(main_location.getAutoindex() == "" ? "off" : main_location.getAutoindex());
    main_location.setClientMaxBodySize(main_location.getClientMaxBodySize() == -1 ? -1 : main_location.getClientMaxBodySize());
    main_location.setCgiParams(main_location.getCgiParams().empty() ? std::map<std::string, std::string>() : main_location.getCgiParams());
    main_location.setErrorPages(main_location.getErrorPages().empty() ? std::map<int, std::string>() : main_location.getErrorPages());
    main_location.setIndex(main_location.getIndex() == "" ? "index.html" : main_location.getIndex());
    main_location.setReturn(main_location.getReturn().first == -1 ? std::pair<int, std::string>(-1, "") : main_location.getReturn());
    main_location.setRoot(main_location.getRoot() == "" ? "" : main_location.getRoot());

    std::vector<std::string> DEFAULT_ALLOW_METHODS;
    DEFAULT_ALLOW_METHODS.push_back("GET");
    DEFAULT_ALLOW_METHODS.push_back("HEAD");

    std::vector<std::string> allowMethods = main_location.getAllowMethods();
    if (allowMethods.empty()) {
        main_location.setAllowMethods(DEFAULT_ALLOW_METHODS);
    }
    
    /**
     * Reassign the main location block to the locations map
     */
    this->main_location = main_location;
    this->locations = locations;
}

Config::~Config()
{
}

enum LocationType {
    EXACT,
    PREFIX,
};

struct LocationConfig {
    LocationType type;
    std::string path;

    LocationConfig(LocationType t, const std::string& p) : type(t), path(p) {}
};

bool exactMatch(const std::string& requestPath, const std::string& configPath) {
    return requestPath == configPath;
}

bool prefixMatch(const std::string& requestPath, const std::string& configPath) {
    // split the requestPath by '/'
    std::vector<std::string> requestPathParts = split(requestPath, '/');
    std::vector<std::string> configPathParts = split(configPath, '/');

    if (requestPathParts.size() < configPathParts.size()) {
        return false;
    }

    for (size_t i = 0; i < configPathParts.size(); i++) {
        if (requestPathParts[i] != configPathParts[i]) {
            return false;
        }
    }

    return true;

    // return requestPath.compare(0, configPath.size(), configPath) == 0 && (requestPath.size() == configPath.size() || requestPath[configPath.size()] == '/');
}

bool matchLocation(const std::string& requestPath, const LocationConfig& location) {
    switch (location.type) {
        case EXACT:
            return exactMatch(requestPath, location.path);
        case PREFIX:
            return prefixMatch(requestPath, location.path);
        default:
            return false;
    }
}


Location Config::getCorrentLocation(std::string path) {
    // first look if there is an exact location
    std::map<std::string, Location, LocationComparator>::iterator it;
    for (it = locations.begin(); it != locations.end(); it++) {
        if (matchLocation(path, LocationConfig(EXACT, it->first))) {
            return it->second;
        }
    }

    // if non of the locations match, look if there is a prefix location
    for (it = locations.begin(); it != locations.end(); it++) {
        if (matchLocation(path, LocationConfig(PREFIX, it->first))) {
            return it->second;
        }
    }

    // if non of the locations match, look if there is a '/' location
    it = locations.find("/");
    if (it != locations.end()) {
        return it->second;
    }

    return main_location;
}

void Config::printConfig() const
{
    std::cout << "***************** Config Start *****************" << std::endl;
    std::cout << "― Listen: " << listen.first << ":" << listen.second << std::endl;
    std::cout << "― Server Names: ";
    for (std::vector<std::string>::const_iterator it = server_names.begin(); it != server_names.end(); ++it)
    {
        std::cout << "― " << *it << " ";
    }
    std::cout << std::endl;
    std::cout << "― Main Location: " << std::endl;
    main_location.printLocation();
    std::cout << "― Locations: " << std::endl;
    for (std::map<std::string, Location, LocationComparator>::const_iterator it = locations.begin(); it != locations.end(); ++it)
    {
        std::cout << "― Key: " << it->first << std::endl;
        it->second.printLocation();
    }
    std::cout << "***************** Config End *****************" << std::endl;
}