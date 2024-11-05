#include "Config.hpp"
#include <iostream>

Config::Config(const std::string &filename)
{
    std::cout << "File name is: " << filename << std::endl;
}

Config::~Config()
{
    std::cout << "Destructor called" << std::endl;
}