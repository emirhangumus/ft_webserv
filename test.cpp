#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <iostream>

std::string GetMachineIpFromInterfaces() {
    // Open the network interfaces directory
    DIR* dir = opendir("/sys/class/net");
    if (!dir) {
        return "Error opening network interfaces";
    }

    std::string ip = "";
    struct dirent* entry;

    // Iterate through network interfaces
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and .. entries
        if (entry->d_name[0] == '.') {
            continue;
        }

        // Create a socket
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            continue;
        }

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(0);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        // Try to bind to check if interface is available
        if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            struct sockaddr_in bound_addr;
            socklen_t addr_len = sizeof(bound_addr);
            
            if (getsockname(sockfd, (struct sockaddr*)&bound_addr, &addr_len) == 0) {
                // Convert IP to string
                unsigned int ip_raw = ntohl(bound_addr.sin_addr.s_addr);
                
                // Skip loopback (127.0.0.1) and empty (0.0.0.0) addresses
                if (ip_raw != 0x7F000001 && ip_raw != 0) {
                    char ip_str[16];
                    int i = 0;
                    
                    // Convert IP octets to string
                    for (int byte = 3; byte >= 0; --byte) {
                        unsigned int octet = (ip_raw >> (byte * 8)) & 0xFF;
                        
                        if (octet >= 100) {
                            ip_str[i++] = '0' + (octet / 100);
                            octet %= 100;
                            ip_str[i++] = '0' + (octet / 10);
                            octet %= 10;
                            ip_str[i++] = '0' + octet;
                        }
                        else if (octet >= 10) {
                            ip_str[i++] = '0' + (octet / 10);
                            ip_str[i++] = '0' + (octet % 10);
                        }
                        else {
                            ip_str[i++] = '0' + octet;
                        }
                        
                        if (byte > 0) {
                            ip_str[i++] = '.';
                        }
                    }
                    ip_str[i] = '\0';
                    
                    ip = std::string(ip_str);
                    close(sockfd);
                    break;
                }
            }
        }
        close(sockfd);
    }

    closedir(dir);
    return ip;
}

int main() {
    std::string ip = GetMachineIpFromInterfaces();
    std::cout << "IP: " << ip << std::endl;
    return 0;
}