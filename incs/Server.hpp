#ifndef SERVER_HPP
#define SERVER_HPP

#include "ConfigParser.hpp"
#include "CacheManager.hpp"
#include "MimeTypes.hpp"
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

    void processConnections(ConfigParser config);
    void closeConnection(int fd, fd_set* master_readfds, fd_set* master_writefds);
    int createSocket(unsigned int port);
    int setNonBlock(int sockfd);
private:
    std::vector<struct sockaddr_in> allSockets;
    std::vector<int> fds;
    CacheManager cacheManager;
    int _maxFd;
    std::vector<int> _listeners;
    std::map<int, std::string> _clientData;

    MimeTypes mimeTypes;

};

#endif