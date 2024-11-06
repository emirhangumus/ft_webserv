#include "Config.hpp"
#include "Utils.hpp"
#include <iostream>

Config::Config()
{
}

void Config::fillConfig(std::string listen, std::vector<std::string> server_names, Location main_location, std::map<std::string, Location> locations)
{
    // std::cout << "Constructor called" << std::endl;
    this->server_names = server_names;
    this->main_location = main_location;
    this->locations = locations;
    std::string delimiter = ":";
    size_t pos = listen.find(delimiter);
    this->listen = std::make_pair(listen.substr(0, pos), stringtoui(listen.substr(pos + 1)));
}

Config::~Config()
{
    // std::cout << "Destructor called" << std::endl;
}