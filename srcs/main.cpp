#include "ConfigParser.hpp"
#include <iostream>
#include <cstdlib>

int main(int ac, char **av)
{
    if (ac != 2)
    {
        std::cerr << "Usage: " << av[0] << " <config_file_path>" << std::endl;
        return 1;
    }

    ConfigParser parser = ConfigParser();
    SRet<std::string> ret = parser.parse(av[1]);
    if (ret.status != EXIT_SUCCESS)
    {
        std::cerr << ret.err << std::endl;
        return 1;
    }

    Config config = parser.get("127.0.0.1:8081");

    if (config.getServerNames().empty())
    {
        std::cerr << "Error: no server names" << std::endl;
        return 1;
    }

    std::vector<std::string> server_names = config.getServerNames();
    std::cout << "Server names: ";
    for (size_t i = 0; i < server_names.size(); i++)
        std::cout << server_names[i] << " ";
    
    std::cout << std::endl;

    return 0;
}