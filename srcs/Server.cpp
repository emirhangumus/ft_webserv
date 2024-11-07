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

Server::Server()
{
}

Server::~Server()
{
}

void setNonBlock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::start(ConfigParser config)
{
	std::vector<struct sockaddr_in>::iterator	so;
	std::vector<unsigned int> allPorts = config.getAllPorts();

	totalPortSize = allPorts.size();
    std::cout << "totalPortSize: " << totalPortSize << std::endl;
	this->allSockets.resize(totalPortSize);
	int	i = 0;
	for (so = this->allSockets.begin(); so != this->allSockets.end(); so++, i++) //configure the sockets
	{
		(*so).sin_family = AF_INET;
		(*so).sin_port = htons(allPorts[i]);
		(*so).sin_addr.s_addr = INADDR_ANY;
		bzero(&((*so).sin_zero), 8);
	}
	const int	enable = 1;
	fds.resize(this->allSockets.size());
	std::vector<int>::iterator	fd;
	for (fd = fds.begin(); fd != fds.end(); fd++) //setup the sockets and make the fds non blocking
	{
		(*fd) = socket(AF_INET, SOCK_STREAM, 0); //IPv4, TCP
		if (*fd < 0)
            throw std::runtime_error("Socket error");
		setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
		setNonBlock(*fd);
	}
	for (fd = fds.begin(), so = this->allSockets.begin(); fd != fds.end(); fd++, so++) //bind the fds to the sockets and put them in listening mode
	{
		if (bind(*fd, (struct sockaddr *)&(*so), sizeof(struct sockaddr_in)))
			throw std::runtime_error("Bind error");
		if (listen(*fd, 25)) //backlog is the length of the queue for the upcoming connections
            throw std::runtime_error("Listen error");
    }
	processConnections(config);
}

void Server::processConnections(ConfigParser config)
{
	std::vector<unsigned int> allPorts = config.getAllPorts();
	// std::vector<pollfd> pollfds;
	// std::vector<pollfd> pollfds = new std::vector<pollfd>;
	std::vector<pollfd> *pollfds = new std::vector<pollfd>;
	(*pollfds).resize(allPorts.size()); //create a vector of pollfds struct, same length as the number of listening sockets
	std::vector<int>::iterator	it = fds.begin();
	std::vector<int>::iterator	ite = fds.end();
	//setup the expected event for the listening sockets to "read"
	for (unsigned int i = 0; it != ite; ++it, ++i)
	{
		(*pollfds)[i].fd = *it;
		(*pollfds)[i].events = POLLIN; //
		(*pollfds)[i].revents = 0;
	}

	// TODO:
	// Remove the unreadable function pointers.
	// See, when the response is actually being sent
	// Main routine. This will be called the whole time the server runs
	while (1) {
		if (poll((struct pollfd *)((*pollfds).data()), (*pollfds).size(), -1) < 1) // Here we wait for poll information.
		{
			std::cout << "An error occured when polling.";
		}
		for (unsigned long i = 0; i < (*pollfds).size(); ++i) //iterate through the entire area of sockets
		{
			if ((*pollfds)[i].revents & POLLIN) //if the socket is readable
			{
				if (i < totalPortSize) //if the socket is a listening socket
				{
					if (acceptConnection((*pollfds), i) == false)
						std::cout << "Error accepting connection" << std::endl;
				}
				else //if the socket is a client socket
				{
					if (readFromClient((*pollfds), i) == false)
						std::cout << "Error reading from client" << std::endl;
				}
			}
			if ((*pollfds)[i].revents & POLLOUT) //if the socket is writable
			{
				if (writeToClient((*pollfds), i, config) == false)
					std::cout << "Error writing to client" << std::endl;
			}
		}
	}
}

bool	Server::acceptConnection(std::vector<pollfd> &pollfds, int i)
{
	struct pollfd tmp;
	socklen_t socketSize = sizeof(this->allSockets[i]);

	std::cout << "Debug" << std::endl;
	int _fd;
	_fd = accept(pollfds[i].fd, (struct sockaddr *)&this->allSockets[i], &socketSize);
	if (_fd == -1) return false;
	std::cout << "Accepting new connection" << i << std::endl;
	tmp.fd = _fd; // set the newly obtained file descriptor to the pollfd. Important to do this before the fcntl call!
	int val = fcntl(_fd, F_SETFL, fcntl(_fd, F_GETFL, 0) | O_NONBLOCK);
	if (val == -1) { // fcntl failed, we now need to close the socket
		close(_fd);
		return false;
	};

	// set the pollfd to listen for POLLIN events (read events)
	tmp.events = POLLIN;
	tmp.revents = 0;
	std::cout << "grep pollfd" << pollfds[i].fd << std::endl;
	std::cout << "grep tmp.fd" << tmp.fd << std::endl;
	pollfds.push_back(tmp); //add the new fd/socket to the set, considered as "client"
	return true;
}

bool	Server::readFromClient(std::vector<pollfd> &pollfds, int i)
{
	std::cout << "Accepted new connection" << std::endl;
	int BUFFER_SIZE_READ = 1024;
	// read the from the client
	char buffer[1024 * 1024];
	std::fill(buffer, buffer + BUFFER_SIZE_READ, 0);
	std::string request;
	while (1) {
		int bytes = recv(pollfds[i].fd, buffer, BUFFER_SIZE_READ, 0);
		if (bytes == -1) {
			std::cout << "Error reading from client: " << strerror(errno) << std::endl;
			break;
		}
		if (bytes == 0) {
			std::cout << "Client closed connection" << std::endl;
			break;
		}
		std::cout << "Read " << bytes << " bytes from client" << std::endl;
		std::cout << "Data: " << buffer << std::endl;

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
	this->requests[pollfds[i].fd] = request;
	pollfds[i].events = POLLOUT;
	return true;
}

bool	Server::writeToClient(std::vector<pollfd> &pollfds, int i, ConfigParser config)
{
	std::string response;
	RequestParser rp = RequestParser(config);
	SRet<bool> ret = rp.parseRequest(this->requests[pollfds[i].fd]);
	if (ret.status == EXIT_FAILURE) {
		std::cout << "Error parsing request: " << ret.err << std::endl;
		response = ErrorResponse::getErrorResponse(400);
	}
	else {
		std::cout << "Parsed request successfully" << std::endl;
		SRet<std::string> realResponse = rp.prepareResponse();
		if (realResponse.status == EXIT_FAILURE) {
			response = realResponse.err;
		}
		else {
			response = realResponse.data;
		}
		// response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\nAccess-Control-Allow-Headers: *\r\n\r\nHello World!";
	}


	// write to the client
	// std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nHello World!";

	// create a redirect response
	// std::string response = "HTTP/1.1 301 Moved Permanently\r\nLocation: https://www.google.com\r\n\r\n";

	// create response with cors.
	// - Access-Control-Allow-Origin
	// - AMethodccess-Control-Allow-s
	// - Access-Control-Allow-Headers

	int bytesWritten = write(pollfds[i].fd, response.c_str(), response.size());
	if (bytesWritten == -1) {
		std::cout << "Error writing to client: " << strerror(errno) << std::endl;
	}
	else {
		std::cout << "Wrote " << bytesWritten << " bytes to client" << std::endl;
	}
	// close the connection
	close(pollfds[i].fd);
	// remove the pollfd from the list
	pollfds[i].events = POLLIN;
	return true;
}
