#include "Server.hpp"
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
#include "RequestParser.hpp"
#include "ErrorResponse.hpp"
#include "CacheManager.hpp"

Server::Server()
{
	this->cacheManager = CacheManager();
	this->mimeTypes = MimeTypes();
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
                        std::cout << "New connection on socket " << newfd << std::endl;
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
                        // Append data to the client's buffer
                        _clientData[fd].append(buf, nbytes);
                        std::cout << "Received " << nbytes << " bytes from client " << fd << std::endl;
                        
                        // Check if we have a complete HTTP request (ending with \r\n\r\n)
                        // Emirhan: Is it the best way to check if the request is complete?
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
                                std::cout << "Content-Length header not found" << std::endl;
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
                    // DEBUG WİTH RED COLOR

                    if (ret.status == EXIT_FAILURE) {
                        std::cout << "Error parsing request: " << ret.err << std::endl;
                        response = ErrorResponse::getErrorResponse(400);
                    } else {
                        SRet<std::string> realResponse = rp.prepareResponse(cacheManager, mimeTypes);
                        response = (realResponse.status == EXIT_FAILURE) ? realResponse.err : realResponse.data;

                        // std::cout << "Response: " << response << std::endl;
                    }
                    // write DEBUG with color red
                    // Handle writing to client
                    // std::cout << "Response: " << response << std::endl;
                    ssize_t bytesWritten = send(fd, response.c_str(), response.size(), 0);
                    delayMilliseconds(1.0); // Sleep for a millisecond to avoid busy waiting
                    if (bytesWritten <= 0) {
                        std::cerr << "Send error: " << strerror(errno) << std::endl;
                        closeConnection(fd, &master_readfds, &master_writefds);
                    } else {
                        std::cout << "Response sent to client " << fd << std::endl;
                    }
                    // TODO:
                    // - Check if the response is fully sent
                    // - Remove the client from the write set if the response is fully sent
                    // - Close the connection if the response is fully sent
                    // - Handle partial writes
                    // - Handle errors
                    // - Handle keep-alive connections
                    // - Handle chunked encoding
                    //
                    // We can use `timeout` system call to set a timeout for the connection
                    std::cout << "Bytes written: " << bytesWritten << std::endl;
                    std::cout << "Response size: " << response.size() << std::endl;
                    if ((size_t)bytesWritten == response.size()) {
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

// bool Server::acceptNewConnectionsIfAvailable(std::vector<pollfd> &pollfds, int i, ConfigParser config) {
// 	struct pollfd tmp;
// 	struct sockaddr_in clientSocket;
// 	socklen_t socketSize = sizeof(clientSocket);

// 	int _fd;
// 	_fd = accept(pollfds[i].fd, (struct sockaddr *)&clientSocket, &socketSize);
// 	if (_fd == -1) return false;
// 	tmp.fd = _fd; // set the newly obtained file descriptor to the pollfd. Important to do this before the fcntl call!
// 	int val = fcntl(_fd, F_SETFL, fcntl(_fd, F_GETFL, 0) | O_NONBLOCK);
// 	if (val == -1) { // fcntl failed, we now need to close the socket
// 		close(_fd);
// 		return false;
// 	};
	
// 	// set the pollfd to listen for POLLIN events (read events)
// 	tmp.events = POLLIN;
// 	tmp.revents = 0;
// 	pollfds.push_back(tmp); //add the new fd/socket to the set, considered as "client"


// 	int BUFFER_SIZE_READ = 1024;
// 	// read the from the client
// 	char buffer[1024]; // 20MB
// 	std::fill(buffer, buffer + BUFFER_SIZE_READ, 0);
// 	std::string request;

// 	int is_error = 0;
// 	while (1) {
// 		// usleep(10); // sleep for a second
// 		int bytes = read(_fd, buffer, BUFFER_SIZE_READ);
// 		if (bytes == -1) {
// 			std::cout << "Error reading from client: " << strerror(errno) << std::endl;
// 			is_error = 1;
// 			break;
// 		}
// 		if (bytes == 0) {
// 			std::cout << "Client closed connection" << std::endl;
// 			break;
// 		}
// 		std::cout << "Read " << bytes << " bytes from client" << std::endl;
// 		// std::cout << "Data: " << buffer << std::endl;

// 		// check if the buffer is empty
// 		if (bytes == 0) {
// 			break;
// 		}

// 		request.append(buffer, bytes);
// 		if (bytes < BUFFER_SIZE_READ) {
// 			break;
// 		}
// 		std::fill(buffer, buffer + BUFFER_SIZE_READ, 0);
// 	}

// 	std::string response;
// 	if (!is_error) {
// 		RequestParser rp = RequestParser(config);
// 		SRet<bool> ret = rp.parseRequest(request);
// 		if (ret.status == EXIT_FAILURE) {
// 			std::cout << "Error parsing request: " << ret.err << std::endl;
// 			response = ErrorResponse::getErrorResponse(400);
// 		}
// 		else {
// 			SRet<std::string> realResponse = rp.prepareResponse(cacheManager);
// 			if (realResponse.status == EXIT_FAILURE) {
// 				response = realResponse.err;
// 				// std::cout << "Error preparing response: " << response << std::endl;
// 			}
// 			else {
// 				response = realResponse.data;
// 			}
// 		}
// 	} else {
// 		std::string response = ErrorResponse::getErrorResponse(500);
// 	}

// 	// cache the response
// 	cacheManager.addCache(request, response);

	
// 	// write to the client
// 	// std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nHello World!";
	
// 	// create a redirect response
// 	// std::string response = "HTTP/1.1 301 Moved Permanently\r\nLocation: https://www.google.com\r\n\r\n";

// 	// create response with cors.
// 	// - Access-Control-Allow-Origin
// 	// - AMethodccess-Control-Allow-s
// 	// - Access-Control-Allow-Headers

// 	int bytesWritten = write(_fd, response.c_str(), response.size());
// 	if (bytesWritten == -1) {
// 		std::cout << "Error writing to client: " << strerror(errno) << std::endl;
// 	}
// 	else {
// 		std::cout << "Wrote " << bytesWritten << " bytes to client" << std::endl;
// 	}
// 	// close the connection
// 	close(_fd);
// 	// remove the pollfd from the list
// 	pollfds.pop_back();

// 	return true;
// }