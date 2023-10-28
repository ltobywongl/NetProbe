#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "es_timer.hpp"

using namespace std;

char *generateMessage(int length, int sequence)
{
    if (length <= 0)
    {
        cout << "Error: generate message length <= 0" << endl;
    }
    char *message = (char *)malloc(length * sizeof(char));
    if (sequence == -1)
    {
        int index = 0;
        for (int i = 0; i < length; i++)
        {
            message[i] = '0' + index;
            index = index + 1;
            if (index == 10)
            {
                index = 0;
            }
        }
        message[length - 1] = '\0';
        return message;
    }
    else
    {
        sprintf(message, "%d", sequence);
        int flag = 0;
        for (int i = 0; i < length; i++)
        {
            if (message[i] == '\0')
            {
                flag = 1;
                message[i] = '#';
                continue;
            }
            if (flag == 1)
            {
                message[i] = '0';
            }
        }
        message[length - 1] = '\0';
        return message;
    }
}

int handleSend(int argc, char *argv[])
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

    if (strcmp(proto, "UDP") == 0)
    {
        // Get prot number
        char buffer[8];
        int r = send(sockfd, "101000", 7, 0);
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

        if (setsockopt(udpsockfd, SOL_SOCKET, SO_SNDBUF, &sbufsize, sizeof(sbufsize)) == -1)
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
        int r = send(sockfd, "001000", 7, 0);
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

    close(sockfd);

    return 0;
}