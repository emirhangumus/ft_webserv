#include "ConfigParser.hpp"
#include "Utils.hpp"
#include <iostream>
#include <fstream>
#include <cstdlib>

ConfigParser::ConfigParser(const std::string &filename)
{
    SRet<std::string> content = readConfigFile(filename);
    if (content.status != EXIT_SUCCESS)
    {
        std::cerr << content.err << std::endl;
        return;
    }
    SRet<std::map<std::string, Config> > config = parseConfigFile(content.data);
    if (config.status != EXIT_SUCCESS)
    {
        std::cerr << config.err << std::endl;
        return;
    }
    m_config = config.data;
}

ConfigParser::~ConfigParser()
{
    std::cout << "Destructor called" << std::endl;
}

SRet<std::string> ConfigParser::readConfigFile(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open())
    {
        return SRet<std::string>(EXIT_FAILURE, "", "Error: could not open file");
    }

    std::string content;
    std::string line;
    while (std::getline(file, line))
    {
        content += line + "\n";
    }

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
    {
        return SRet<std::map<std::string, Config> >(EXIT_FAILURE, config, newContent.err);
    }

    return SRet<std::map<std::string, Config> >(EXIT_SUCCESS, config);
}