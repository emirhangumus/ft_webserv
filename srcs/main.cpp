#include "Server.hpp"
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

    Server server = Server();

    server.processConnections(parser);

    return 0;
}