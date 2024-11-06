#ifndef SERVER_HPP
#define SERVER_HPP

#include "ConfigParser.hpp"
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <map>
#include <vector>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <poll.h>

class Server
{
public:
    Server();
    ~Server();

    void start(ConfigParser config);
    void processConnections(ConfigParser config);
    bool acceptNewConnectionsIfAvailable(std::vector<pollfd> &pollfds, int i, ConfigParser config);
private:
    std::vector<struct sockaddr_in> allSockets;
    std::vector<int> fds;
    unsigned int totalPortSize;
};

#endif // SERVER_HPP