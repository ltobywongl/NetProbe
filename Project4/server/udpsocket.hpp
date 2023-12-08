#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "helper.hpp"

using namespace std;

class UDPSocket {
public:
    int sockfd;

    UDPSocket() : sockfd(-1) {
        memset(&serverAddress, 0, sizeof(serverAddress));
        serverAddressLength = sizeof(serverAddress);
    }

    bool createSocket() {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            cerr << "Error creating socket" << endl;
            return false;
        }
        return true;
    }

    bool bindToPort(int port) {
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddress.sin_port = htons(port);

        if (bind(sockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
            cerr << "Error binding socket" << endl;
            return false;
        }

        serverAddressLength = sizeof(serverAddress);
        return true;
    }

    bool setOption(int level, int optname, const void* optval, socklen_t optlen) {
        if (setsockopt(sockfd, level, optname, optval, optlen) == -1)
        {
            perror("Error setting socket buffer size");
            return false;
        }
        return true;
    }

    int getAddress(sockaddr * destAddress, socklen_t * destLength) {
        return getsockname(sockfd, destAddress, destLength);
    }

    int getPort() {
        struct sockaddr_in udpAddress;
        socklen_t udpAddressLength = sizeof(udpAddress);
        getAddress((struct sockaddr *)&udpAddress, &udpAddressLength);
        return (int)ntohs(udpAddress.sin_port);
    }

    ssize_t send(char* message, size_t messageLength, sockaddr_in* clientAddress) {
        return sendto(sockfd, message, messageLength, 0, (struct sockaddr *)&clientAddress, sizeof(clientAddress));
    }

    bool receive(char* buffer, size_t bufferSize, sockaddr_in* clientAddress, socklen_t* clientAddressLength) {
        ssize_t rec = recvfrom(sockfd, buffer, bufferSize - 1, 0, (struct sockaddr *)clientAddress, clientAddressLength);
        if (rec <= 0) {
            perror("Receive failed");
            return false;
        }
        buffer[rec] = '\0';
        return true;
    }

    void closeSocket() {
        if (sockfd >= 0) {
            close(sockfd);
            sockfd = -1;
        }
    }

private:
    struct sockaddr_in serverAddress;
    socklen_t serverAddressLength;
};