
#ifdef WIN32 // Windows
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "es_timer.hpp"
#include "helper.hpp"
#else // Linux
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "es_timer.hpp"
#include "helper.hpp"
#endif

using namespace std;

#ifdef WIN32
int handleResponse(int argc, char *argv[])
{
    int stat = 500;
    char *rhost = new char[256];
    strcpy(rhost, "localhost");
    int rport = 4180;
    char *proto = new char[4];
    strcpy(proto, "UDP");
    int pktsize = 1000;
    int pktrate = 1000;
    int pktnum = 0;
    int sbufsize = 65536;
    int rbufsize = 65536;
    char *p;

    // Read options
    for (int i = 2; i < argc; i++)
    {
        if (i + 1 < argc && argv[i][0] == '-')
        {
            if (strcmp(argv[i], "-stat") == 0)
            {
                stat = strtol(argv[i + 1], &p, 10);
                i++;
            }
            else if (strcmp(argv[i], "-rhost") == 0)
            {
                rhost = argv[i + 1];
                i++;
            }
            else if (strcmp(argv[i], "-rport") == 0)
            {
                rport = strtol(argv[i + 1], &p, 10);
                i++;
            }
            else if (strcmp(argv[i], "-proto") == 0)
            {
                proto = argv[i + 1];
                i++;
            }
            else if (strcmp(argv[i], "-pktsize") == 0)
            {
                pktsize = strtol(argv[i + 1], &p, 10);
                i++;
            }
            else if (strcmp(argv[i], "-pktrate") == 0)
            {
                pktrate = strtol(argv[i + 1], &p, 10);
                i++;
            }
            else if (strcmp(argv[i], "-pktnum") == 0)
            {
                pktnum = strtol(argv[i + 1], &p, 10);
                i++;
            }
            else if (strcmp(argv[i], "-sbufsize") == 0)
            {
                sbufsize = strtol(argv[i + 1], &p, 10);
                i++;
            }
            else if (strcmp(argv[i], "-rbufsize") == 0)
            {
                rbufsize = strtol(argv[i + 1], &p, 10);
                i++;
            }
            else
            {
                fprintf(stderr, "Unknown option: %s\n", argv[i]);
                return -1;
            }
        }
    }
    if (strcmp(rhost, "localhost") == 0)
        strcpy(rhost, "127.0.0.1");

    // Init winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("Failed to initialize Winsock\n");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(rport);
    server_addr.sin_addr.s_addr = inet_addr(rhost);

    SOCKET sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == INVALID_SOCKET)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char *)&sbufsize, sizeof(sbufsize)) == SOCKET_ERROR)
    {
        perror("Error setting socket buffer size");
    }

    // Connect to the server
    if (connect(sockfd, (const struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == SOCKET_ERROR)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    if (strcmp(proto, "UDP") == 0)
    {
        // Get prot number
        char buffer[8];
        char *data = (char *)malloc(8 * sizeof(char));
        sprintf(data, "10%d", pktrate);
        int r = send(sockfd, data, 7, 0);
        if (r <= 0)
        {
            perror("Send failed");
            exit(EXIT_FAILURE);
        }

        int s = recv(sockfd, buffer, sizeof(buffer), 0);
        if (s > 0)
        {
            cout << "Received port number: " << buffer << endl;
        }
        else if (s == 0)
        {
            cout << "Server disconnected." << endl;
        }
        else
        {
            perror("Receive failed");
        }

        // Create new address
        struct sockaddr_in server_udp_addr;
        memset(&server_udp_addr, 0, sizeof(struct sockaddr_in));

        // Configure server address
        server_udp_addr.sin_family = AF_INET;
        server_udp_addr.sin_port = htons(strtol(buffer, &p, 10));
        server_udp_addr.sin_addr.s_addr = inet_addr(rhost);

        // Create UDP socket
        int udpsockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (udpsockfd == -1)
        {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }

        if (setsockopt(udpsockfd, SOL_SOCKET, SO_SNDBUF, (const char *)&sbufsize, sizeof(sbufsize)) == -1)
        {
            perror("Error setting socket buffer size");
        }

        // Send data
        long msgsent = 0;
        ES_FlashTimer clock;
        int packetNum = 0;
        long previousClock = clock.Elapsed();
        long rateLimitClock = clock.Elapsed();
        long initialClock = clock.Elapsed();
        long cumTimeCost = 0, cumBytesSent = 0, statTime = 0, bytesSentSecond = 0;
        while (pktnum == 0 || msgsent < pktnum)
        {
            int bytes_sent = 0;
            char *message = generateMessage(pktsize, msgsent + 1);
            while (bytes_sent < pktsize)
            {
                if ((bytesSentSecond <= pktrate) || (pktrate == 0))
                {
                    int r = sendto(udpsockfd, message + bytes_sent, pktsize - bytes_sent, 0, (struct sockaddr *)&server_udp_addr, sizeof(server_udp_addr));
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
            cumBytesSent += bytes_sent;
            msgsent++;
            free(message);

            // Handle time
            long currentClock = clock.Elapsed();
            double timeCost = currentClock - previousClock;
            previousClock = currentClock;
            cumTimeCost += timeCost;

            // Stats
            statTime += timeCost;

            if (statTime >= stat)
            {
                double throughput = (double)(cumBytesSent * 8) / (cumTimeCost * 1000);
                printf("Receiver: [Elapsed] %ld ms, [Pkts] %ld, [Rate] %.2f Mbps\n", currentClock - initialClock, msgsent, throughput);
                statTime = 0;
            }
        }
    }
    else
    {
        // TCP
        char *data = (char *)malloc(8 * sizeof(char));
        sprintf(data, "00%d", pktrate);
        int r = send(sockfd, data, 7, 0);
        if (r <= 0)
        {
            perror("Send failed");
            exit(EXIT_FAILURE);
        }

        long msgsent = 0;
        ES_FlashTimer clock;
        int packetNum = 0;
        long previousClock = clock.Elapsed();
        long rateLimitClock = clock.Elapsed();
        long initialClock = clock.Elapsed();
        long cumTimeCost = 0, cumBytesSent = 0, statTime = 0, bytesSentSecond = 0;
        while (pktnum == 0 || msgsent < pktnum)
        {
            // Send data to the server
            int bytes_sent = 0;
            char *message = generateMessage(pktsize, msgsent + 1);
            while (bytes_sent < pktsize)
            {
                if ((bytesSentSecond < pktrate) || (pktrate == 0))
                {
                    int r = send(sockfd, message + bytes_sent, pktsize - bytes_sent, 0);
                    if (r > 0)
                        bytes_sent += r;
                    else
                    {
                        free(message);
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
            cumBytesSent += bytes_sent;
            msgsent++;
            free(message);

            // Handle time
            long currentClock = clock.Elapsed();
            double timeCost = currentClock - previousClock;
            previousClock = currentClock;
            cumTimeCost += timeCost;

            // Stats
            statTime += timeCost;

            if (statTime >= stat)
            {
                double throughput = (double)(cumBytesSent * 8) / (cumTimeCost * 1000);
                printf("Receiver: [Elapsed] %ld ms, [Pkts] %ld, [Rate] %.2f Mbps\n", currentClock - initialClock, msgsent, throughput);
                statTime = 0;
            }
        }
    }

    closesocket(sockfd);
    WSACleanup();

    return 0;
}

#else // Linux

int handleResponse(int argc, char *argv[])
{
    int stat = 500;
    char *rhost = new char[256];
    strcpy(rhost, "localhost");
    int rport = 4180;
    char *persist = new char[4];
    strcpy(persist, "no\0");
    int pktsize = 1000;
    int pktrate = 10;
    int pktnum = 0;
    int sbufsize = 65536;
    int rbufsize = 65536;
    char *p;

    // Read options
    for (int i = 2; i < argc; i++)
    {
        if (i + 1 < argc && argv[i][0] == '-')
        {
            if (strcmp(argv[i], "-stat") == 0)
            {
                stat = strtol(argv[i + 1], &p, 10);
                i++;
            }
            else if (strcmp(argv[i], "-rhost") == 0)
            {
                rhost = argv[i + 1];
                i++;
            }
            else if (strcmp(argv[i], "-rport") == 0)
            {
                rport = strtol(argv[i + 1], &p, 10);
                i++;
            }
            else if (strcmp(argv[i], "-persist") == 0)
            {
                persist = argv[i + 1];
                i++;
            }
            else if (strcmp(argv[i], "-pktsize") == 0)
            {
                pktsize = strtol(argv[i + 1], &p, 10);
                i++;
            }
            else if (strcmp(argv[i], "-pktrate") == 0)
            {
                pktrate = strtol(argv[i + 1], &p, 10);
                i++;
            }
            else if (strcmp(argv[i], "-pktnum") == 0)
            {
                pktnum = strtol(argv[i + 1], &p, 10);
                i++;
            }
            else if (strcmp(argv[i], "-sbufsize") == 0)
            {
                sbufsize = strtol(argv[i + 1], &p, 10);
                i++;
            }
            else if (strcmp(argv[i], "-rbufsize") == 0)
            {
                rbufsize = strtol(argv[i + 1], &p, 10);
                i++;
            }
            else
            {
                fprintf(stderr, "Unknown option: %s\n", argv[i]);
                return -1;
            }
        }
    }
    if (strcmp(rhost, "localhost") == 0)
        strcpy(rhost, "127.0.0.1");

    int sockfd;
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(rport);
    server_addr.sin_addr.s_addr = inet_addr(rhost);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sbufsize, sizeof(sbufsize)) == -1)
    {
        perror("Error setting socket buffer size");
    }

    // Connect to the server
    if (connect(sockfd, (const struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    if (strcmp(persist, "no") == 0)
    {
        // Get prot number
        char buffer[pktsize];
        char *data = (char *)malloc(8 * sizeof(char));
        sprintf(data, "20%d", pktsize);
        int r = send(sockfd, data, 7, 0);
        if (r <= 0)
        {
            perror("Send failed");
            exit(EXIT_FAILURE);
        }

        int s = recv(sockfd, buffer, sizeof(buffer), 0);
        if (s > 0)
        {
            cout << "Received port number: " << buffer << endl;
        }
        else if (s == 0)
        {
            cout << "Server disconnected." << endl;
        }
        else
        {
            perror("Receive failed");
        }

        // Create new address
        struct sockaddr_in server_response_addr;
        memset(&server_response_addr, 0, sizeof(struct sockaddr_in));

        // Configure server address
        server_response_addr.sin_family = AF_INET;
        server_response_addr.sin_port = htons(strtol(buffer, &p, 10));
        server_response_addr.sin_addr.s_addr = inet_addr(rhost);

        long messageResponse = 0;
        ES_FlashTimer clock;
        long previousClock = clock.Elapsed();
        long rateLimitClock = clock.Elapsed();
        long initialClock = clock.Elapsed();
        int minTime = -1, maxTime = -1;
        long cumTimeCost = 0, statTime = 0, messagesSentSecond = 0, cumJitter = 0;
        while (pktnum == 0 || messageResponse < pktnum)
        {
            int bytes_sent = 0;
            char *message = generateMessage(pktsize, messageResponse + 1);

            // Make new connection everytime
            // cout << "Initializing socket" << endl;
            int tcpSockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (tcpSockfd == -1)
            {
                perror("Socket creation failed");
                exit(EXIT_FAILURE);
            }
            if (setsockopt(tcpSockfd, SOL_SOCKET, SO_SNDBUF, &sbufsize, sizeof(sbufsize)) == -1)
            {
                perror("Error setting socket buffer size");
            }
            if (connect(tcpSockfd, (const struct sockaddr *)&server_response_addr, sizeof(struct sockaddr)) == -1)
            {
                perror("Connection failed");
                exit(EXIT_FAILURE);
            }

            // cout << "Sending message" << endl;
            // Send message
            while (bytes_sent < pktsize)
            {
                if ((messagesSentSecond <= pktrate) || (pktrate == 0))
                {
                    int r = send(tcpSockfd, message + bytes_sent, pktsize - bytes_sent, 0);
                    if (r > 0)
                    {
                        bytes_sent += r;
                    }
                    else
                    {
                        perror("Send failed");
                        break;
                    }
                }
                if (clock.Elapsed() - 1000 > rateLimitClock)
                {
                    messagesSentSecond = 0;
                    rateLimitClock = clock.Elapsed();
                }
            }

            // Waiting for response
            // cout << "Waiting for response" << endl;
            long bytesReceived = 0;
            while (bytesReceived < pktsize)
            {
                int ret = recv(tcpSockfd, buffer, pktsize - bytesReceived, 0);
                if (ret <= 0)
                {
                    cerr << "Receive failed" << endl;
                    break;
                }
                else bytesReceived += ret;
            }

            messageResponse++;
            free(message);
            close(tcpSockfd);

            // Handle time
            long currentClock = clock.Elapsed();
            double timeCost = currentClock - previousClock;

            // Check max min time
            if (timeCost < minTime || minTime == -1)
            {
                minTime = timeCost;
            }
            else if (timeCost > maxTime || maxTime == -1)
            {
                maxTime = timeCost;
            }

            previousClock = currentClock;
            cumTimeCost += timeCost;
            double averageTime = 0;
            if (cumTimeCost > 0)
                averageTime = cumTimeCost / messageResponse;
            cumJitter += timeCost - averageTime;

            // Stats
            statTime += timeCost;

            if (statTime >= stat)
            {
                double jitter = (double)cumJitter / messageResponse;
                printf("Elapsed [%lds] Replies [%ld] Min [%dms] Max [%dms] Avg [%ldms] Jitter [%lfms]\n", (currentClock - initialClock) / 1000, messageResponse, minTime, maxTime, cumTimeCost / messageResponse, jitter);
                statTime = 0;
            }
        }
    }
    else
    {
        // Persistant
        char buffer[pktsize];
        char *data = (char *)malloc(8 * sizeof(char));
        sprintf(data, "21%d", pktrate);
        int r = send(sockfd, data, 7, 0);
        if (r <= 0)
        {
            perror("Send failed");
            exit(EXIT_FAILURE);
        }

        long messageResponse = 0;
        ES_FlashTimer clock;
        long previousClock = clock.Elapsed();
        long rateLimitClock = clock.Elapsed();
        long initialClock = clock.Elapsed();
        int minTime = -1, maxTime = -1;
        long cumTimeCost = 0, statTime = 0, messagesSentSecond = 0, cumJitter = 0;
        while (pktnum == 0 || messageResponse < pktnum)
        {
            int bytes_sent = 0;
            char *message = generateMessage(pktsize, messageResponse + 1);

            // Send message
            while (bytes_sent < pktsize)
            {
                if ((messagesSentSecond <= pktrate) || (pktrate == 0))
                {
                    int r = send(sockfd, message + bytes_sent, pktsize - bytes_sent, 0);
                    if (r > 0)
                    {
                        bytes_sent += r;
                    }
                    else
                    {
                        perror("Send failed");
                        break;
                    }
                }
                if (clock.Elapsed() - 1000 > rateLimitClock)
                {
                    messagesSentSecond = 0;
                    rateLimitClock = clock.Elapsed();
                }
            }

            // Waiting for response
            // cout << "Waiting for response" << endl;
            long bytesReceived = 0;
            while (bytesReceived < pktsize)
            {
                int ret = recv(sockfd, buffer, pktsize - bytesReceived, 0);
                if (ret <= 0)
                {
                    cerr << "Receive failed" << endl;
                    break;
                }
                else bytesReceived += ret;
            }

            messageResponse++;
            free(message);

            // Handle time
            long currentClock = clock.Elapsed();
            double timeCost = currentClock - previousClock;

            // Check max min time
            if (timeCost < minTime || minTime == -1)
            {
                minTime = timeCost;
            }
            else if (timeCost > maxTime || maxTime == -1)
            {
                maxTime = timeCost;
            }

            previousClock = currentClock;
            cumTimeCost += timeCost;
            double averageTime = 0;
            if (cumTimeCost > 0)
                averageTime = cumTimeCost / messageResponse;
            cumJitter += timeCost - averageTime;

            // Stats
            statTime += timeCost;

            if (statTime >= stat)
            {
                double jitter = (double)cumJitter / messageResponse;
                printf("Elapsed [%lds] Replies [%ld] Min [%dms] Max [%dms] Avg [%ldms] Jitter [%lfms]\n", (currentClock - initialClock) / 1000, messageResponse, minTime, maxTime, cumTimeCost / messageResponse, jitter);
                statTime = 0;
            }
        }

        close(sockfd);
    }
    return 0;
}
#endif
