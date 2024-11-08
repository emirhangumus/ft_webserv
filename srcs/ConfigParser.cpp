#include "ConfigParser.hpp"
#include "Config.hpp"
#include "Location.hpp"
#include "Utils.hpp"
#include <iostream>
#include <fstream>
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
    _VALID_DIRECTIVES.push_back("cgi_params");
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

std::map<std::string, std::string> parseCgiParams(const std::string& value) {
    std::map<std::string, std::string> cgi_params;
    std::vector<std::string> cgi_params_values = split(value, ' ');
    
    for (size_t i = 0; i < cgi_params_values.size(); i++) {
        std::vector<std::string> cgi_param = split(cgi_params_values[i], ',');
        if (cgi_param.size() != 2) {
            throw std::runtime_error("Error: invalid config file: cgi_params directive must have 2 arguments");
        }
        // Store in correct order: key = cgi_param[0], value = cgi_param[1]
        cgi_params[cgi_param[1]] = cgi_param[0];  // Note the reversed order from your original code
    }
    return cgi_params;
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
    std::map<std::string, Location, LocationComparator> locations;

    std::string current_location_key;
    bool is_default_config_set = false;

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
                if (!is_default_config_set) {
                    defaultConfig = current_config;
                    is_default_config_set = true;
                }

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
            std::string value = trim(line.substr(space_pos + 1));

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

                    // location_key have to start with '/'
                    if (current_location_key[0] != '/')
                        return SRet<std::map<std::string, Config> >(EXIT_FAILURE, config, "Error: invalid config file: location key must start with '/'");
                    // location_key can't have '/' at the end
                    if (current_location_key.size() > 1 && current_location_key[current_location_key.size() - 1] == '/')
                        return SRet<std::map<std::string, Config> >(EXIT_FAILURE, config, "Error: invalid config file: location key can't end with '/'");

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
                    std::vector<std::string> return_values = split(value, ' ');
                    if (return_values.size() != 2)
                        return SRet<std::map<std::string, Config> >(EXIT_FAILURE, config, "Error: invalid config file: return directive must have 2 argument. First argument is return code, second is return value");
                    if (stringtoui(return_values[0]) < 100 || stringtoui(return_values[0]) > 599)
                        return SRet<std::map<std::string, Config> >(EXIT_FAILURE, config, "Error: invalid config file: return code must be between 100 and 599");
                    std::pair<int, std::string> return_ = std::make_pair(stringtoui(return_values[0]), return_values[1]);
                    main_location.setReturn(return_);
                } else if (directive == "client_max_body_size") {
                    main_location.setClientMaxBodySize(convertSizeToBytes(value));
                } else if (directive == "allow_methods") {
                    main_location.setAllowMethods(split(value, ' '));
                } else if (directive == "cgi_params") {
                    std::map<std::string, std::string> params = parseCgiParams(value);
                    main_location.setCgiParams(params);
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
                    std::vector<std::string> return_values = split(value, ' ');
                    if (return_values.size() != 2)
                        return SRet<std::map<std::string, Config> >(EXIT_FAILURE, config, "Error: invalid config file: return directive must have 2 argument. First argument is return code, second is return value");
                    if (stringtoui(return_values[0]) < 100 || stringtoui(return_values[0]) > 599)
                        return SRet<std::map<std::string, Config> >(EXIT_FAILURE, config, "Error: invalid config file: return code must be between 100 and 599");
                    std::pair<int, std::string> return_ = std::make_pair(stringtoi(return_values[0]), return_values[1]);
                    locations[current_location_key].setReturn(return_);
                } else if (directive == "client_max_body_size") {
                    locations[current_location_key].setClientMaxBodySize(convertSizeToBytes(value));
                } else if (directive == "allow_methods") {
                    locations[current_location_key].setAllowMethods(split(value, ' '));
                } else if (directive == "cgi_params") {
                    std::map<std::string, std::string> params = parseCgiParams(value);
                    locations[current_location_key].setCgiParams(params);
                    
                    // // Debug printing
                    // std::cout << "Original map contents:" << std::endl;
                    // for (std::map<std::string, std::string>::const_iterator it = params.begin(); 
                    //     it != params.end(); ++it) {
                    //     std::cout << it->first << " = " << it->second << std::endl;
                    // }
                    
                    std::cout << "\nStored map contents:" << std::endl;
                    const std::map<std::string, std::string>& stored_params = 
                        locations[current_location_key].getCgiParams();
                    for (std::map<std::string, std::string>::const_iterator it = stored_params.begin(); 
                        it != stored_params.end(); ++it) {
                        std::cout << it->first << " = " << it->second << std::endl;
                    }
                } else if (directive == "error_page") {
                    locations[current_location_key].setErrorPage(value);
                }
            }
        }
    }

    // print all locations's all cgi_params via Config
    // for (std::map<std::string, Config>::const_iterator config_it = config.begin(); 
    //          config_it != config.end(); ++config_it) 
    //     {
    //         const std::map<std::string, Location, LocationComparator>& locations = 
    //             config_it->second.getLocations();
            
    //         // Iterate over locations
    //         for (std::map<std::string, Location, LocationComparator>::const_iterator loc_it = locations.begin(); 
    //              loc_it != locations.end(); ++loc_it) 
    //         {
    //             const std::map<std::string, std::string>& cgi_params = 
    //                 loc_it->second.getCgiParams();
                
    //             // Iterate over CGI parameters
    //             for (std::map<std::string, std::string>::const_iterator param_it = cgi_params.begin(); 
    //                  param_it != cgi_params.end(); ++param_it) 
    //             {
    //                 std::cout << "\033[1;34m" << param_it->first << " = " 
    //                          << param_it->second << "\033[0m" << std::endl;
    //             }
    //         }
    //     }
    


    if (deep_level != 0) {
        return SRet<std::map<std::string, Config> >(EXIT_FAILURE, config, "Error: invalid config file: too many '{'");
    }

    return SRet<std::map<std::string, Config> >(EXIT_SUCCESS, config);
}

std::vector<unsigned int> ConfigParser::getAllPorts() const
{
    std::vector<unsigned int> ports;
    for (std::map<std::string, Config>::const_iterator it = m_config.begin(); it != m_config.end(); it++)
        ports.push_back(it->second.getListen().second);
    return ports;
}

Config ConfigParser::getServerConfig(const std::string &host) const
{
    for (std::map<std::string, Config>::const_iterator it = m_config.begin(); it != m_config.end(); it++)
    {
        std::cout << "Now checking: " << it->first << " vs " << host << std::endl;
        if (it->first == host)
        {
            std::cout << "Found: " << it->first << std::endl;
            return it->second;
        }
    }
    return defaultConfig;
}