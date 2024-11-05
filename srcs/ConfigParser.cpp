#include "ConfigParser.hpp"
#include "Config.hpp"
#include "Location.hpp"
#include "Utils.hpp"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <map>

ConfigParser::ConfigParser()
{
    std::vector<std::string> _VALID_DIRECTIVES;
    _VALID_DIRECTIVES.push_back("listen");
    _VALID_DIRECTIVES.push_back("server");
    _VALID_DIRECTIVES.push_back("server_name");
    _VALID_DIRECTIVES.push_back("location");
    _VALID_DIRECTIVES.push_back("root");
    _VALID_DIRECTIVES.push_back("index");
    _VALID_DIRECTIVES.push_back("try_files");
    _VALID_DIRECTIVES.push_back("autoindex");
    _VALID_DIRECTIVES.push_back("return");
    _VALID_DIRECTIVES.push_back("client_max_body_size");
    _VALID_DIRECTIVES.push_back("allow_methods");
    _VALID_DIRECTIVES.push_back("limit_except");
    _VALID_DIRECTIVES.push_back("cgi_path");
    _VALID_DIRECTIVES.push_back("error_page");
    VALID_DIRECTIVES = _VALID_DIRECTIVES;
}

ConfigParser::~ConfigParser()
{
    // std::cout << "Destructor called" << std::endl;
}

Config ConfigParser::get(const std::string &key) const
{
    // print all map keys
    for (std::map<std::string, Config>::const_iterator it = m_config.begin(); it != m_config.end(); it++)
        std::cout << it->first << std::endl;



    // check if key exists
    std::map<std::string, Config>::const_iterator it = m_config.find(key);
    if (it == m_config.end())
        return Config();
    return it->second;
}

SRet<std::string> ConfigParser::parse(const std::string &filename)
{
    SRet<std::string> content = readConfigFile(filename);
    
    if (content.status != EXIT_SUCCESS)
        return content;
    
    SRet<std::map<std::string, Config> > config = parseConfigFile(content.data);
    
    if (config.status != EXIT_SUCCESS)
        return SRet<std::string>(EXIT_FAILURE, "", config.err);

    m_config = config.data;
    return SRet<std::string>(EXIT_SUCCESS, "");
}

SRet<std::string> ConfigParser::readConfigFile(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open())
        return SRet<std::string>(EXIT_FAILURE, "", "Error: could not open file");

    std::string content;
    std::string line;
    while (std::getline(file, line))
        content += line + "\n";

    file.close();

    return SRet<std::string>(EXIT_SUCCESS, content);
}

SRet<std::string> ConfigParser::removeComments(const std::string &content)
{
    std::string newContent;
    std::string line;
    std::vector<std::string> values = split(content, '\n');

    for (size_t i = 0; i < values.size(); i++)
    {
        line = trim(values[i]);
        if (line.empty() || line[0] == CONFIG_COMMENT_CHAR)
            continue;
        newContent += line + "\n";
    }

    if (newContent.empty())
        return SRet<std::string>(EXIT_FAILURE, "", "Error: no content");

    if (newContent[newContent.size() - 1] == '\n')
        newContent = newContent.substr(0, newContent.size() - 1);
    return SRet<std::string>(EXIT_SUCCESS, newContent);
}

SRet<std::map<std::string, Config> > ConfigParser::parseConfigFile(const std::string &content)
{
    std::map<std::string, Config> config;
    SRet<std::string> newContent = removeComments(content);
    if (newContent.status != EXIT_SUCCESS)
        return SRet<std::map<std::string, Config> >(EXIT_FAILURE, config, newContent.err);

    std::vector<std::string> values = split(newContent.data, '\n');

    int deep_level = 0; // level 1 is server, level 2 is location

    // Config current_config;
    std::string listen;
    std::vector<std::string> server_names;
    Location main_location;
    std::map<std::string, Location> locations;

    std::string current_location_key;

    for (size_t i = 0; i < values.size(); i++)
    {
        std::string line = values[i];
        size_t space_pos = line.find_first_of(' ');
        if (space_pos == std::string::npos) {
            if (line[line.size() - 1] == '}') {
                deep_level--;
            } else {
                if (deep_level == 0 && line.size() > 0) {
                    return SRet<std::map<std::string, Config> >(EXIT_FAILURE, config, "Error: invalid config file: no server block");
                }
            }

            if (deep_level < 0) {
                return SRet<std::map<std::string, Config> >(EXIT_FAILURE, config, "Error: invalid config file: too many '}'");
            }

            if (deep_level == 0) {
                if (listen.empty()) {
                    return SRet<std::map<std::string, Config> >(EXIT_FAILURE, config, "Error: invalid config file: listen directive is missing");
                }

                Config current_config = Config();
                current_config.fillConfig(listen, server_names, main_location, locations);
                config[listen] = current_config;

                listen = "";
                server_names.clear();
                main_location = Location();
                locations.clear();
                current_location_key = "";
            }

        } else {
            if (line[line.size() - 1] == '{') {
                if (deep_level == 2) {
                    return SRet<std::map<std::string, Config> >(EXIT_FAILURE, config, "Error: invalid config file: too many '{'");
                }
                if (deep_level == 0 && line.substr(0, space_pos) != "server") {
                    return SRet<std::map<std::string, Config> >(EXIT_FAILURE, config, "Error: invalid config file: have to start with server block");
                }
                deep_level++;
            }

            if (deep_level < 1 || deep_level > 2) {
                return SRet<std::map<std::string, Config> >(EXIT_FAILURE, config, "Error: invalid config file: too many '{'");
            }

            std::string directive = line.substr(0, space_pos);
            std::string value = line.substr(space_pos + 1);

            std::vector<std::string>::iterator it = std::find(VALID_DIRECTIVES.begin(), VALID_DIRECTIVES.end(), directive);
            if (it == VALID_DIRECTIVES.end()) {
                return SRet<std::map<std::string, Config> >(EXIT_FAILURE, config, "Error: invalid directive: " + directive);
            }

            if (deep_level == 1 || directive == "location") {
                if (directive == "listen") {
                    listen = value;
                } else if (directive == "server_name") {
                    server_names = split(value, ' ');
                } else if (directive == "location") {
                    if (line[line.size() - 1] != '{')
                        return SRet<std::map<std::string, Config> >(EXIT_FAILURE, config, "Error: invalid config file: location block must end with '{'");
                    
                    std::vector<std::string> location_values = split(value, ' ');
                    if (location_values.size() != 2)
                        return SRet<std::map<std::string, Config> >(EXIT_FAILURE, config, "Error: invalid config file: location directive must have 2 arguments");
                    
                    current_location_key = location_values[0];
                    locations[current_location_key] = Location();
                    locations[current_location_key].setPath(current_location_key);
                } else if (directive == "root") {
                    main_location.setRoot(value);
                } else if (directive == "index") {
                    main_location.setIndex(value);
                } else if (directive == "try_files") {
                    main_location.setTryFiles(value);
                } else if (directive == "autoindex") {
                    main_location.setAutoindex(value);
                } else if (directive == "return") {
                    main_location.setReturn(value);
                } else if (directive == "client_max_body_size") {
                    main_location.setClientMaxBodySize(value);
                } else if (directive == "allow_methods") {
                    main_location.setAllowMethods(split(value, ' '));
                } else if (directive == "limit_except") {
                    main_location.setLimitExcept(value);
                } else if (directive == "cgi_path") {
                    main_location.setCgiPath(value);
                } else if (directive == "error_page") {
                    main_location.setErrorPage(value);
                }
            } else if (deep_level == 2) {
                if (directive == "root") {
                    locations[current_location_key].setRoot(value);
                } else if (directive == "index") {
                    locations[current_location_key].setIndex(value);
                } else if (directive == "try_files") {
                    locations[current_location_key].setTryFiles(value);
                } else if (directive == "autoindex") {
                    locations[current_location_key].setAutoindex(value);
                } else if (directive == "return") {
                    locations[current_location_key].setReturn(value);
                } else if (directive == "client_max_body_size") {
                    locations[current_location_key].setClientMaxBodySize(value);
                } else if (directive == "allow_methods") {
                    locations[current_location_key].setAllowMethods(split(value, ' '));
                } else if (directive == "limit_except") {
                    locations[current_location_key].setLimitExcept(value);
                } else if (directive == "cgi_path") {
                    locations[current_location_key].setCgiPath(value);
                } else if (directive == "error_page") {
                    locations[current_location_key].setErrorPage(value);
                }
            }
        }
    }

    if (deep_level != 0) {
        return SRet<std::map<std::string, Config> >(EXIT_FAILURE, config, "Error: invalid config file: too many '{'");
    }

    return SRet<std::map<std::string, Config> >(EXIT_SUCCESS, config);
}
