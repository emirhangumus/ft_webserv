#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <map>
#include <vector>
#include "Utils.hpp"
#include "Config.hpp"

#define CONFIG_COMMENT_CHAR '#'

/**
 * also, the `listen` directive is unique,
 * so the map key is listen.
 */

class ConfigParser
{
public:

    ConfigParser();
    ~ConfigParser();

    // the key is the `listen` directive
    Config get(const std::string &key) const;
    SRet<std::string> parse(const std::string &filename);

    std::vector<unsigned int> getAllPorts() const;
    Config getServerConfig(const std::string &host) const;

private:
    SRet<std::string> readConfigFile(const std::string &filename);
    SRet<std::string> removeComments(const std::string &content);
    SRet<std::map<std::string, Config> > parseConfigFile(const std::string &content);
    std::map<std::string, Config> m_config;
    Config defaultConfig;
    std::vector<std::string> VALID_DIRECTIVES;
};

#endif