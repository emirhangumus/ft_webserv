#include "ConfigParser.hpp"
#include <iostream>

int main(int ac, char **av)
{
    if (ac != 2)
    {
        std::cerr << "Usage: " << av[0] << " <config_file_path>" << std::endl;
        return 1;
    }

    ConfigParser parser = ConfigParser(av[1]);
    return 0;
}