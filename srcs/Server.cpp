#include "Server.hpp"
#include "RequestParser.hpp"
#include "ErrorResponse.hpp"
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
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cerrno>
#include <cstring>

Server::Server()
{
	this->mimeTypes = MimeTypes();
	this->_maxFd = 0;
}

Server::~Server()
{
    for (std::vector<int>::iterator it = _listeners.begin(); it != _listeners.end(); it++) {
        close(*it);
    }
}

int Server::setNonBlock(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int Server::createSocket(unsigned int port) {
    // Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cerr << "Failed to create socket for port " << port << ": "
                  << strerror(errno) << std::endl;
        return -1;
    }

    // Set both SO_REUSEADDR and SO_REUSEPORT
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "Failed to set SO_REUSEADDR for port " << port << ": "
                  << strerror(errno) << std::endl;
        close(sockfd);
        return -1;
    }

    #ifdef SO_REUSEPORT
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        std::cerr << "Warning: Failed to set SO_REUSEPORT for port " << port << ": "
                  << strerror(errno) << std::endl;
    }
    #endif

    // Set linger option to force socket closure
    struct linger lin;
    lin.l_onoff = 1;
    lin.l_linger = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (const char *)&lin, sizeof(lin)) == -1) {
        std::cerr << "Warning: Failed to set SO_LINGER for port " << port << ": "
                  << strerror(errno) << std::endl;
    }

    // Prepare the sockaddr_in structure
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    // Add retry logic for bind
    int retry_count = 0;
    const int max_retries = 3;
    while (retry_count < max_retries) {
        if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
            if (errno == EADDRINUSE) {
                std::cerr << "Retry " << retry_count + 1 << "/" << max_retries 
                         << ": Port " << port << " is in use, waiting..." << std::endl;
                sleep(1);  // Wait a second before retry
                retry_count++;
                continue;
            }
            std::cerr << "Failed to bind socket for port " << port << ": "
                      << strerror(errno) << std::endl;
            close(sockfd);
            return -1;
        }
        break;  // Successful bind
    }

    if (retry_count == max_retries) {
        std::cerr << "Failed to bind after " << max_retries << " attempts for port " 
                  << port << std::endl;
        close(sockfd);
        return -1;
    }

    // Listen
    if (listen(sockfd, SOMAXCONN) == -1) {
        std::cerr << "Failed to listen on port " << port << ": "
                  << strerror(errno) << std::endl;
        close(sockfd);
        return -1;
    }

    std::cout << "Successfully created listener on port " << port << std::endl;
    return sockfd;
}

// Helper function to set non-blocking mode
void delayMilliseconds(double milliseconds) {
    clock_t start = clock();
    // Calculate target duration in terms of clock ticks
    clock_t duration = static_cast<clock_t>(milliseconds * (CLOCKS_PER_SEC / 1000.0));
    while (clock() - start < duration);
}

std::string readRequestBody(int client_fd, size_t contentLength) {
    std::string body;
    char buffer[8192];  // Larger buffer size for reading
    ssize_t bytesRead;
    size_t totalRead = 0;

    // Read in a loop until content length is fully read
    while (totalRead < contentLength) {
        bytesRead = read(client_fd, buffer, sizeof(buffer));
        if (bytesRead <= 0) break;  // Exit on error or EOF
        body.append(buffer, bytesRead);
        totalRead += bytesRead;
    }

    return body;
}

void Server::processConnections(ConfigParser config)
{
    std::vector<unsigned int> allPorts = config.getAllPorts();
    fd_set master_readfds;    // Master file descriptor list
    fd_set master_writefds;   // Master file descriptor list for writing
    fd_set read_fds;          // Temp file descriptor list for select()
    fd_set write_fds;         // Temp file descriptor list for select()
    
    // Clear the master and temp sets
    FD_ZERO(&master_readfds);
    FD_ZERO(&master_writefds);
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    // Add the listener to the master set
    for (std::vector<unsigned int>::iterator port = allPorts.begin(); port != allPorts.end(); port++)
    {
        int listener = createSocket(*port);
        if (listener < 0) {
            continue;
        }
        FD_SET(listener, &master_readfds);
        if (listener > _maxFd) {
            _maxFd = listener;
        }
        _listeners.push_back(listener);  // Store listeners for cleanup
    }
    // color red debug
    // Main loop
    while (true) {
        read_fds = master_readfds;    // Copy the master set
        write_fds = master_writefds;  // Copy the master write set

        // Wait for activity on any socket
        int selectResult = select(_maxFd + 1, &read_fds, &write_fds, NULL, NULL);
        if (selectResult == -1) {
            if (errno == EINTR) {  // Handle interrupted system call
                continue;
            }
            std::cerr << "Select error: " << strerror(errno) << std::endl;
            break;
        }

        // Handle all sockets with activity
        for (int fd = 0; fd <= _maxFd; fd++) {
            // delayMilliseconds(1.0); // Sleep for a millisecond to avoid busy waiting
            if (FD_ISSET(fd, &read_fds)) {
                // Check if this is a listener socket
                if (std::find(_listeners.begin(), _listeners.end(), fd) != _listeners.end()) {
                    // Handle new connection
                    struct sockaddr_in clientAddr;
                    socklen_t clientLen = sizeof(clientAddr);
                    int newfd = accept(fd, (struct sockaddr *)&clientAddr, &clientLen);
                    
                    if (newfd == -1) {
                        std::cerr << "Accept error: " << strerror(errno) << std::endl;
                    } else {
                        setNonBlock(newfd);
                        FD_SET(newfd, &master_readfds);     // Add to master set
                        if (newfd > _maxFd) {
                            _maxFd = newfd;
                        }
                        _clientData[newfd] = "";  // Initialize client data
                    }
                } else {
                    // Handle data from a client
                    char buf[4096 * 4];  // 16KB buffer
                    ssize_t nbytes = recv(fd, buf, sizeof(buf), 0);

                    if (nbytes <= 0) {
                        if (nbytes < 0) {
                            std::cerr << "Recv error: " << strerror(errno) << std::endl;
                        }
                        closeConnection(fd, &master_readfds, &master_writefds);
                    } else {
                        _clientData[fd].append(buf, nbytes);

                        if (_clientData[fd].find("\r\n\r\n") != std::string::npos) {
                            RequestParser rp = RequestParser(config);
                            rp.parseRequest(_clientData[fd]);

                            std::string method = rp.getMethod();

                            if (method == "GET") 
                            {
                                FD_SET(fd, &master_writefds);  // Mark for writing
                                continue;
                            }

                            try {
                                std::string cL = rp.getHeaders().at("Content-Length");

                                if (cL == "") {
                                    std::cout << "Content-Length header is empty" << std::endl;
                                    closeConnection(fd, &master_readfds, &master_writefds);
                                    continue;
                                }

                                size_t contentLength = stringtoui(cL);

                                if (_clientData[fd].size() < contentLength + 4) {
                                    std::cout << "Incomplete request" << std::endl;
                                    continue;
                                }
                                FD_SET(fd, &master_writefds);  // Mark for writing
                            }
                            catch (const std::out_of_range& oor) {
                                FD_SET(fd, &master_writefds);  // Mark for writing
                            }
                        }
                    }
                }
            }
            
            if (FD_ISSET(fd, &write_fds)) {
                if (!_clientData[fd].empty()) {
                    std::string response;
                    RequestParser rp = RequestParser(config);
                    SRet<bool> ret = rp.parseRequest(_clientData[fd]);

                    if (ret.status == EXIT_FAILURE) {
                        std::cout << "Error parsing request: " << ret.err << std::endl;
                        response = ErrorResponse::getErrorResponse(400);
                    } else {
                        SRet<std::string> realResponse = rp.prepareResponse(mimeTypes);
                        response = (realResponse.status == EXIT_FAILURE) ? realResponse.err : realResponse.data;
                    }

                    const ssize_t CHUNK_SIZE = 8192;  // 8KB chunks
                    ssize_t totalBytesWritten = 0;
                    const ssize_t totalBytes = response.size();

                    while (totalBytesWritten < totalBytes) {
                        const ssize_t bytesToWrite = std::min(CHUNK_SIZE, totalBytes - totalBytesWritten);
                        const ssize_t bytesWritten = send(fd, response.data() + totalBytesWritten, bytesToWrite, 0);
                        
                        if (bytesWritten < 0) {
                            if (errno == EINTR) {  // Handle interrupt
                                continue;
                            }
                            std::cerr << "Send error: " << strerror(errno) << std::endl;
                            closeConnection(fd, &master_readfds, &master_writefds);
                            break;
                        }
                        
                        if (bytesWritten == 0) {  // Connection closed
                            std::cerr << "Connection closed by peer" << std::endl;
                            closeConnection(fd, &master_readfds, &master_writefds);
                            break;
                        }
                        
                        totalBytesWritten += bytesWritten;
                    }

                    if (totalBytesWritten == static_cast<ssize_t>(response.size())) {
                        shutdown(fd, SHUT_WR); // Signals end of data transmission
                        closeConnection(fd, &master_readfds, &master_writefds);
                    }
                }
            }
        }
    }
}


// Helper method to close connections
void Server::closeConnection(int fd, fd_set* master_readfds, fd_set* master_writefds) {
    _clientData.erase(fd);
    close(fd);
    FD_CLR(fd, master_readfds);
    FD_CLR(fd, master_writefds);
}
