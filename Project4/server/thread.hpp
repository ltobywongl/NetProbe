#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "es_timer.hpp"
#include "udpsocket.hpp"

#define MAX_CONN 10
#define TIMEOUT_SECONDS 10

using namespace std;

struct ThreadData
{
    sockaddr_in client_addr;
    int params;
    int sockfd;
    int lport;
    int pktrate;
    int bufsize;
    SSL_CTX **sslContext;
};

string readFileIntoString(const string& path) {
    ifstream input_file(path);
    if (!input_file.is_open()) {
        cerr << "Cannot open file: '" << path << "'" << endl;
        return "Cannot open file";
    }
    stringstream buffer;
    buffer << input_file.rdbuf();
    return buffer.str();
}

void handleConnection(ThreadData *data)
{
    int params = data->params;
    int sockfd = data->sockfd;
    int pktrate = data->pktrate;
    short exitFlag = 0;
    char buffer[data->bufsize];
    struct sockaddr_in udp_addr;
    socklen_t addr_len = sizeof(udp_addr);

    if (params >= 30) {
        if (params == 31) {
            SSL_CTX *sslContext = *(data->sslContext);
            SSL* ssl = SSL_new(sslContext);
            if (!ssl) {
                cerr << "Failed to create SSL object" << endl;
                return;
            }

            SSL_set_fd(ssl, sockfd);

            // Perform the SSL/TLS handshake
            int sslHandshakeResult = SSL_accept(ssl);
            if (sslHandshakeResult != 1) {
                cerr << "Failed SSL/TLS handshake" << endl;
                return;
            }
            
            int bytesRead = SSL_read(ssl, buffer, sizeof(buffer) - 1);
            if (bytesRead <= 0) {
                cerr << "Failed to read request" << endl;
                return;
            }

            string request(buffer, bytesRead);

            int pathStartPos = request.find_first_of(' ') + 1;
            int i = pathStartPos + 1;
            while (request[i] != ' ') ++i;

            string path = request.substr(pathStartPos, i - pathStartPos);

            // Default to index.html if no file specified
            if (path == "/") path = "index.html";
            else if (path[path.length()-1] == '/') path = path + "index.html";

            string file = readFileIntoString(path);

            string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + to_string(file.size()) + "\r\n\r\n" + file;

            int bytesSent = SSL_write(ssl, response.c_str(), response.length());
            if (bytesSent <= 0) {
                cerr << "Failed to send response" << endl;
                return;
            }

            close(sockfd);
        } else if (params == 30) {
            int bytesRead = read(sockfd, buffer, sizeof(buffer) - 1);
            if (bytesRead <= 0) {
                cerr << "Failed to read request" << endl;
                return;
            }

            string request(buffer, bytesRead);

            int pathStartPos = request.find_first_of(' ') + 1;
            int i = pathStartPos + 1;
            while (request[i] != ' ') ++i;

            string path = request.substr(pathStartPos, i - pathStartPos);
            // Default to index.html if no file specified
            if (path == "/") path = "index.html";
            else if (path[path.length()-1] == '/') path = path + "index.html";

            string file = readFileIntoString(path);

            string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + to_string(file.size()) + "\r\n\r\n" + file;

            long bytesSent = send(sockfd, response.c_str(), response.length(), 0);
            if (bytesSent == -1) {
                cerr << "Failed to send response" << endl;
            }

            close(sockfd);
        }
    }
    else if (params >= 20)
    {
        // pktrate used for size in this mode
        int pktsize = pktrate;
        if (params == 20)
        {
            // No Persist
            // Setup new port for the connection
            int tcpSockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (tcpSockfd == -1) {
                cerr << "Failed to create socket" << endl;
                cout << "Closing TCP Socket..." << endl;
                close(sockfd);
                return;
            }

            struct sockaddr_in serverAddress;
            memset(&serverAddress, 0, sizeof(serverAddress));
            serverAddress.sin_family = AF_INET;
            serverAddress.sin_addr.s_addr = INADDR_ANY;
            serverAddress.sin_port = htons(0);

            if (bind(tcpSockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
                cerr << "Failed to bind the socket" << endl;
                cout << "Closing TCP Sockets..." << endl;
                close(sockfd);
                close(tcpSockfd);
                return;
            }

            if (listen(tcpSockfd, 3) == -1) {
                cerr << "Failed to listen for connections" << endl;
                cout << "Closing TCP Sockets..." << endl;
                close(sockfd);
                close(tcpSockfd);
                return;
            }

            setsockopt(tcpSockfd, SOL_SOCKET, SO_SNDBUF, &data->bufsize, sizeof(data->bufsize));
            setsockopt(tcpSockfd, SOL_SOCKET, SO_RCVBUF, &data->bufsize, sizeof(data->bufsize));

            struct sockaddr_in tcpAddress;
            socklen_t tcpAddressLength = sizeof(tcpAddress);
            getsockname(tcpSockfd, (struct sockaddr *)&tcpAddress, &tcpAddressLength);
            string msg = to_string(ntohs(tcpAddress.sin_port));
            send(sockfd, msg.c_str(), msg.length(), 0);
            cout << "Sent TCP port number to client: " << msg << endl;

            // Accepting and handling connection
            struct sockaddr_in client_addr;
            memset(&client_addr, 0, sizeof(struct sockaddr_in));
            socklen_t client_addr_len = sizeof(client_addr);
            while (true)
            {
                fd_set readfds;
                FD_ZERO(&readfds);
                FD_SET(tcpSockfd, &readfds);
                FD_SET(sockfd, &readfds);

                struct timeval timeout;
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;

                int activity = select(max(sockfd, tcpSockfd) + 1, &readfds, nullptr, nullptr, &timeout);
                if (activity < 0)
                {
                    perror("Select failed");
                    break;
                }

                if (FD_ISSET(sockfd, &readfds))
                {
                    // TCP socket check
                    int ret = recv(sockfd, buffer, data->bufsize, 0);
                    if (ret <= 0)
                    {
                        printf("Client Disconnected or Error\n");
                        exitFlag = 1;
                        break;
                    }
                }
                else
                {
                    // TCP socket error check
                    int option = 0;
                    socklen_t option_len = sizeof(option);
                    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &option, &option_len) == -1 || option != 0)
                    {
                        perror("Client disconnected or Error");
                        exitFlag = 1;
                        break;
                    }
                }

                if (FD_ISSET(tcpSockfd, &readfds))
                {
                    int newsockfd = accept(tcpSockfd, (struct sockaddr *)&client_addr, &client_addr_len);
                    if (newsockfd < 0)
                    {
                        perror("Accept failed");
                        break;
                    }

                    char buffer[pktsize];
                    int ret = recv(newsockfd, buffer, pktsize, 0);
                    if (ret <= 0)
                    {
                        printf("Client Disconnected or Error\n");
                        exitFlag = 1;
                        break;
                    }
                    int r = send(newsockfd, buffer, pktsize, 0);
                    if (r <= 0)
                    {
                        cout << "Client Disconnected or Error" << endl;
                        exitFlag = 1;
                        break;
                    }
                    close(newsockfd);
                }
            }
            close(tcpSockfd);
        }
        else
        {
            //Persist
            if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &data->bufsize, sizeof(data->bufsize)) == -1)
            {
                perror("Error setting socket buffer size");
            }

            char buffer[pktsize];
            while (exitFlag == 0)
            {
                int ret = recv(sockfd, buffer, pktsize, 0);
                if (ret <= 0)
                {
                    printf("Client Disconnected or Error\n");
                    exitFlag = 1;
                    break;
                }
                int r = send(sockfd, buffer, pktsize, 0);
                if (r <= 0)
                {
                    cout << "Client Disconnected or Error" << endl;
                    exitFlag = 1;
                    break;
                }
            }
        }
    }
    else if (params >= 10)
    {
        if (params == 10)
        {
            // UDP -send
            UDPSocket udpSocket;
            if (!udpSocket.createSocket())
            {
                return;
            }
            if (!udpSocket.bindToPort(0))
            {
                close(udpSocket.sockfd);
                close(sockfd);
            }
            udpSocket.setOption(SOL_SOCKET, SO_RCVBUF, &data->bufsize, sizeof(data->bufsize));

            string msg = to_string(udpSocket.getPort());
            send(sockfd, msg.c_str(), msg.length(), 0);
            cout << "Sent UDP port number to client: " << msg << endl;

            struct sockaddr_in clientAddress;
            memset(&clientAddress, 0, sizeof(struct sockaddr_in));
            socklen_t clientLength = sizeof(clientAddress);

            while (exitFlag == 0)
            {
                struct timeval timeout;
                timeout.tv_sec = 5;
                timeout.tv_usec = 0;

                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(udpSocket.sockfd, &fds);
                FD_SET(sockfd, &fds);

                int maxfd = max(udpSocket.sockfd, sockfd) + 1;
                int select_ret = select(maxfd, &fds, nullptr, nullptr, &timeout);

                if (select_ret == -1)
                {
                    perror("Select failed");
                    exitFlag = 1;
                    break;
                }

                if (FD_ISSET(udpSocket.sockfd, &fds))
                {
                    if (!udpSocket.receive(buffer, data->bufsize, &clientAddress, &clientLength))
                    {
                        exitFlag = 1;
                        break;
                    }
                }

                if (FD_ISSET(sockfd, &fds))
                {
                    // TCP socket check
                    int ret = recv(sockfd, buffer, data->bufsize, 0);
                    if (ret <= 0)
                    {
                        printf("Client Disconnected or Error\n");
                        exitFlag = 1;
                        break;
                    }
                }
                else
                {
                    // TCP socket error check
                    int option = 0;
                    socklen_t option_len = sizeof(option);
                    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &option, &option_len) == -1 || option != 0)
                    {
                        perror("Client disconnected or Error");
                        exitFlag = 1;
                        break;
                    }
                }
            }

            cout << "Closing UDP Socket..." << endl;
            close(udpSocket.sockfd);
        }
        else
        {
            // UDP -recv
            char *p;

            int udpsockfd = socket(AF_INET, SOCK_DGRAM, 0);
            if (udpsockfd == -1)
            {
                perror("Socket creation failed");
                exit(EXIT_FAILURE);
            }

            if (setsockopt(udpsockfd, SOL_SOCKET, SO_SNDBUF, &data->bufsize, sizeof(data->bufsize)) == -1)
            {
                perror("Error setting socket buffer size");
            }

            // Client UDP
            struct sockaddr_in client_udp_addr;
            memset(&client_udp_addr, 0, sizeof(struct sockaddr_in));
            client_udp_addr.sin_family = AF_INET;
            client_udp_addr.sin_port = htons(data->lport + 1);
            client_udp_addr.sin_addr = data->client_addr.sin_addr;

            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 500;

            // Send data
            long msgsent = 0;
            ES_FlashTimer clock;
            int packetNum = 0;
            int pktsize = 1000;
            long rateLimitClock = clock.Elapsed();
            long bytesSentSecond = 0;

            while (exitFlag == 0)
            {
                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(udpsockfd, &fds);
                FD_SET(sockfd, &fds);

                int maxfd = max(udpsockfd, sockfd) + 1;
                int select_ret = select(maxfd, &fds, nullptr, nullptr, &timeout);

                if (select_ret == -1)
                {
                    perror("Select failed");
                    exitFlag = 1;
                    break;
                }

                // cout << FD_ISSET(udpsockfd, &fds) << ", " << FD_ISSET(sockfd, &fds) << endl;

                int bytes_sent = 0;
                char *message = generateMessage(pktsize, msgsent + 1);
                while (bytes_sent < pktsize)
                {
                    if ((bytesSentSecond <= pktrate) || (pktrate == 0))
                    {
                        int r = sendto(udpsockfd, message + bytes_sent, pktsize - bytes_sent, 0, (struct sockaddr *)&client_udp_addr, sizeof(client_udp_addr));
                        if (r > 0)
                        {
                            bytes_sent += r;
                        }
                        else
                        {
                            perror("Send failed");
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (clock.Elapsed() - 1000 > rateLimitClock)
                    {
                        bytesSentSecond = 0;
                        rateLimitClock = clock.Elapsed();
                    }
                }
                bytesSentSecond += bytes_sent;
                msgsent++;
                free(message);

                if (FD_ISSET(sockfd, &fds))
                {
                    // TCP socket check
                    int ret = recv(sockfd, buffer, data->bufsize, 0);
                    if (ret <= 0)
                    {
                        perror("Client Disconnected or Error");
                        exitFlag = 1;
                        break;
                    }
                }
                else
                {
                    // TCP socket error check
                    int option = 0;
                    socklen_t option_len = sizeof(option);
                    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &option, &option_len) == -1 || option != 0)
                    {
                        perror("Client Disconnected or Error");
                        exitFlag = 1;
                        break;
                    }
                }
            }
            cout << "Closing UDP Socket..." << endl;
            close(udpsockfd);
        }
    }
    else
    {
        // TCP -send
        if (params == 0)
        {
            if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &data->bufsize, sizeof(data->bufsize)) == -1)
            {
                perror("Error setting socket buffer size");
            }

            while (exitFlag == 0)
            {
                long bytesReceived = 0;
                while (bytesReceived < data->bufsize)
                {
                    int ret = recv(sockfd, buffer, data->bufsize - bytesReceived, 0);
                    if (ret == -1)
                    {
                        perror("Receive failed");
                        exitFlag = 1;
                        break;
                    }
                    else if (ret == 0)
                    {
                        printf("Client Disconnected\n");
                        exitFlag = 1;
                        break;
                    }
                    bytesReceived += ret;
                }
                if (exitFlag == 1)
                    break;
            }
        }
        else // TCP -recv
        {
            if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &data->bufsize, sizeof(data->bufsize)) == -1)
            {
                perror("Error setting socket buffer size");
            }

            long msgsent = 0;
            ES_FlashTimer clock;
            int pktsize = 1000;
            long rateLimitClock = clock.Elapsed();
            long bytesSentSecond = 0;
            while (exitFlag == 0)
            {
                // Send data to the server
                int bytes_sent = 0;
                char *message = generateMessage(pktsize, msgsent + 1);
                while (bytes_sent < pktsize && exitFlag == 0)
                {
                    if ((bytesSentSecond < pktrate) || (pktrate == 0))
                    {
                        int r = send(sockfd, message + bytes_sent, pktsize - bytes_sent, MSG_NOSIGNAL);
                        if (r > 0)
                            bytes_sent += r;
                        else
                        {
                            cout << "Client Disconnected" << endl;
                            exitFlag = 1;
                            break;
                        }
                    }
                    if (clock.Elapsed() - 1000 > rateLimitClock)
                    {
                        bytesSentSecond = 0;
                        rateLimitClock = clock.Elapsed();
                    }
                }
                bytesSentSecond += bytes_sent;
                msgsent++;
                free(message);
            }
        }
    }
    cout << "Closing TCP Socket..." << endl;
    close(sockfd);
    return;
}