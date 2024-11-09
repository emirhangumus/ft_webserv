#include "Server.hpp"
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
#include "RequestParser.hpp"
#include "ErrorResponse.hpp"
#include "CacheManager.hpp"

Server::Server()
{
	this->cacheManager = CacheManager();
	this->_maxFd = 0;
}

Server::~Server()
{
	this->cacheManager.clearCache();
}

int Server::setNonBlock(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}


void Server::start(ConfigParser config)
{
	std::vector<struct sockaddr_in>::iterator	so;
	std::vector<unsigned int> allPorts = config.getAllPorts();

	totalPortSize = allPorts.size();
    std::cout << "totalPortSize: " << totalPortSize << std::endl;
	allSockets.resize(totalPortSize);
	int	i = 0;
	for (so = allSockets.begin(); so != allSockets.end(); so++, i++) //configure the sockets
	{
		(*so).sin_family = AF_INET;
		(*so).sin_port = htons(allPorts[i]);
		(*so).sin_addr.s_addr = INADDR_ANY;
		bzero(&((*so).sin_zero), 8);
	}
	const int	enable = 1;
	fds.resize(allSockets.size());
	std::vector<int>::iterator	fd;
	for (fd = fds.begin(); fd != fds.end(); fd++) //setup the sockets and make the fds non blocking
	{
		(*fd) = socket(AF_INET, SOCK_STREAM, 0); //IPv4, TCP
		if (*fd < 0)
            throw std::runtime_error("Socket error");
		setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
		setNonBlock(*fd);
	}
	this->_maxFd = *std::max_element(fds.begin(), fds.end());
	for (fd = fds.begin(), so = allSockets.begin(); fd != fds.end(); fd++, so++) //bind the fds to the sockets and put them in listening mode
	{
		if (bind(*fd, (struct sockaddr *)&(*so), sizeof(struct sockaddr_in)))
			throw std::runtime_error("Bind error");
		if (listen(*fd, 25)) //backlog is the length of the queue for the upcoming connections
            throw std::runtime_error("Listen error");
    }
	processConnections(config);
}

int Server::createSocket(unsigned int port) {
    // Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
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


void Server::processConnections(ConfigParser config)
{
    std::vector<unsigned int> allPorts = config.getAllPorts();
    fd_set master_readfds;    // Master file descriptor list
    fd_set master_writefds;   // Master file descriptor list for writing
    fd_set read_fds;         // Temp file descriptor list for select()
    fd_set write_fds;        // Temp file descriptor list for select()
    
    // Clear the master and temp sets
    FD_ZERO(&master_readfds);
    FD_ZERO(&master_writefds);
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);

    // Add the listener to the master set
    for (const auto& port : allPorts) {
        int listener = createSocket(port);
        if (listener < 0) {
            continue;
        }
        FD_SET(listener, &master_readfds);
        if (listener > _maxFd) {
            _maxFd = listener;
        }
        _listeners.push_back(listener);  // Store listeners for cleanup
    }

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
                        FD_SET(newfd, &master_writefds);    // Also monitor for writing
                        if (newfd > _maxFd) {
                            _maxFd = newfd;
                        }
                        std::cout << "New connection on socket " << newfd << std::endl;
                    }
                } else {
                    // Handle data from a client
                    char buf[4096];
                    ssize_t nbytes = recv(fd, buf, sizeof(buf), 0);
                    
                    if (nbytes <= 0) {
                        // Got error or connection closed
                        if (nbytes < 0) {
                            std::cerr << "Recv error: " << strerror(errno) << std::endl;
                        }
                        closeConnection(fd, &master_readfds, &master_writefds);
                    } else {
                        // Process received data here
                        // For now, we'll just echo back
                        FD_SET(fd, &master_writefds);  // Mark for writing
                    }
                }
            }
            
            if (FD_ISSET(fd, &write_fds)) {
                // Handle writing to client
                std::string response = "HTTP/1.1 200 OK\r\n"
                                     "Content-Type: text/plain\r\n"
                                     "Content-Length: 12\r\n"
                                     "\r\n"
                                     "Hello World!";
                
                ssize_t bytesWritten = send(fd, response.c_str(), response.size(), 0);
                if (bytesWritten <= 0) {
                    if (bytesWritten < 0) {
                        std::cerr << "Send error: " << strerror(errno) << std::endl;
                    }
                    closeConnection(fd, &master_readfds, &master_writefds);
                } else {
                    // Successfully wrote data, remove from write set
                    FD_CLR(fd, &master_writefds);
                }
            }
        }
    }
}

// Helper method to close connections
void Server::closeConnection(int fd, fd_set* master_readfds, fd_set* master_writefds) {
    close(fd);
    FD_CLR(fd, master_readfds);
    FD_CLR(fd, master_writefds);
}
bool Server::acceptNewConnectionsIfAvailable(std::vector<pollfd> &pollfds, int i, ConfigParser config) {
	struct pollfd tmp;
	struct sockaddr_in clientSocket;
	socklen_t socketSize = sizeof(clientSocket);

	int _fd;
	_fd = accept(pollfds[i].fd, (struct sockaddr *)&clientSocket, &socketSize);
	if (_fd == -1) return false;
	tmp.fd = _fd; // set the newly obtained file descriptor to the pollfd. Important to do this before the fcntl call!
	int val = fcntl(_fd, F_SETFL, fcntl(_fd, F_GETFL, 0) | O_NONBLOCK);
	if (val == -1) { // fcntl failed, we now need to close the socket
		close(_fd);
		return false;
	};
	
	// set the pollfd to listen for POLLIN events (read events)
	tmp.events = POLLIN;
	tmp.revents = 0;
	pollfds.push_back(tmp); //add the new fd/socket to the set, considered as "client"


	int BUFFER_SIZE_READ = 1024;
	// read the from the client
	char buffer[1024]; // 20MB
	std::fill(buffer, buffer + BUFFER_SIZE_READ, 0);
	std::string request;

	int is_error = 0;
	while (1) {
		// usleep(10); // sleep for a second
		int bytes = read(_fd, buffer, BUFFER_SIZE_READ);
		if (bytes == -1) {
			std::cout << "Error reading from client: " << strerror(errno) << std::endl;
			is_error = 1;
			break;
		}
		if (bytes == 0) {
			std::cout << "Client closed connection" << std::endl;
			break;
		}
		std::cout << "Read " << bytes << " bytes from client" << std::endl;
		// std::cout << "Data: " << buffer << std::endl;

		// check if the buffer is empty
		if (bytes == 0) {
			break;
		}

		request.append(buffer, bytes);
		if (bytes < BUFFER_SIZE_READ) {
			break;
		}
		std::fill(buffer, buffer + BUFFER_SIZE_READ, 0);
	}

	std::string response;
	if (!is_error) {
		RequestParser rp = RequestParser(config);
		SRet<bool> ret = rp.parseRequest(request);
		if (ret.status == EXIT_FAILURE) {
			std::cout << "Error parsing request: " << ret.err << std::endl;
			response = ErrorResponse::getErrorResponse(400);
		}
		else {
			SRet<std::string> realResponse = rp.prepareResponse(cacheManager);
			if (realResponse.status == EXIT_FAILURE) {
				response = realResponse.err;
				// std::cout << "Error preparing response: " << response << std::endl;
			}
			else {
				response = realResponse.data;
			}
		}
	} else {
		std::string response = ErrorResponse::getErrorResponse(500);
	}

	// cache the response
	cacheManager.addCache(request, response);

	
	// write to the client
	// std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nHello World!";
	
	// create a redirect response
	// std::string response = "HTTP/1.1 301 Moved Permanently\r\nLocation: https://www.google.com\r\n\r\n";

	// create response with cors.
	// - Access-Control-Allow-Origin
	// - AMethodccess-Control-Allow-s
	// - Access-Control-Allow-Headers

	int bytesWritten = write(_fd, response.c_str(), response.size());
	if (bytesWritten == -1) {
		std::cout << "Error writing to client: " << strerror(errno) << std::endl;
	}
	else {
		std::cout << "Wrote " << bytesWritten << " bytes to client" << std::endl;
	}
	// close the connection
	close(_fd);
	// remove the pollfd from the list
	pollfds.pop_back();

	return true;
}