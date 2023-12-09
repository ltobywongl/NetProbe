#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "es_timer.hpp"

long getSequence(char *message)
{
    char *p;
    char sequence[16];
    strncpy(sequence, message, 15);
    sequence[15] = '\0';
    for (int i = 0; i < 15; i++)
    {
        if (sequence[i] == '#')
        {
            sequence[i] = '\0';
            break;
        }
    }
    return strtol(sequence, &p, 10);
}

int handleRecv(int argc, char *argv[])
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
        char *data = (char *)malloc(8 * sizeof(char));
        sprintf(data, "11%d", pktrate);
        int r = send(sockfd, data, 7, 0);
        if (r <= 0)
        {
            perror("Send failed");
            exit(EXIT_FAILURE);
        }

        // UDP Recv
        int udpsockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (udpsockfd == -1)
        {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }

        if (setsockopt(udpsockfd, SOL_SOCKET, SO_RCVBUF, &rbufsize, sizeof(rbufsize)) == -1)
        {
            perror("Error setting socket buffer size");
        }

        // Create new address
        struct sockaddr_in client_udp_addr;
        memset(&client_udp_addr, 0, sizeof(struct sockaddr_in));

        // Configure server address
        client_udp_addr.sin_family = AF_INET;
        client_udp_addr.sin_port = htons(rport + 1);
        client_udp_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        // Bind the socket to the address and port
        if (bind(udpsockfd, (struct sockaddr *)&client_udp_addr, sizeof(struct sockaddr_in)) == -1)
        {
            perror("Bind failed");
            exit(EXIT_FAILURE);
        }

        printf("Client UDP listening on port %d...\n", rport + 1);

        char buffer[rbufsize];
        ES_FlashTimer clock;
        int packetNum = 0;
        long previousClock = clock.Elapsed();
        long initialClock = clock.Elapsed();
        long cumTimeCost = 0, cumBytesReceived = 0, statTime = 0, cumJitter = 0;
        while (1)
        {
            socklen_t addr_len = sizeof(server_addr);

            // Receive data from the server
            int ret = recvfrom(udpsockfd, buffer, rbufsize - 1, 0, (struct sockaddr *)&server_addr, &addr_len);
            if (ret == -1)
            {
                perror("Receive failed");
                exit(EXIT_FAILURE);
            }
            cumBytesReceived += ret;

            // Handle time
            long currentClock = clock.Elapsed();
            double timeCost = currentClock - previousClock;
            previousClock = currentClock;
            cumTimeCost += timeCost;

            // Calculate
            packetNum++;
            double averageTime = 0;
            if (cumTimeCost > 0)
                averageTime = cumTimeCost / packetNum;
            cumJitter += timeCost - averageTime;

            // Stats
            statTime += timeCost;

            if (statTime >= stat)
            {
                long currentPacket = getSequence(buffer);
                double throughput = (double)(cumBytesReceived * 8) / (cumTimeCost * 1000);
                double jitter = (double)cumJitter / packetNum;
                printf("Receiver: [Elapsed] %ld ms, [Pkts] %d, [Lost] %ld, %.2f%%, [Rate] %.2f Mbps, [Jitter] %.6f ms\n", currentClock - initialClock, packetNum, currentPacket - packetNum, (double)100 * (currentPacket - packetNum) / currentPacket, throughput, jitter);
                statTime = 0;
            }
        }

        // Close the server socket
        close(udpsockfd);
    }
    else
    {
        // TCP recv
        char *data = (char *)malloc(8 * sizeof(char));
        sprintf(data, "01%d", pktrate);
        int r = send(sockfd, data, 7, 0);
        if (r <= 0)
        {
            perror("Send failed");
            exit(EXIT_FAILURE);
        }

        char buffer[rbufsize];
        ES_FlashTimer clock;
        short exitFlag = 0;
        int packetNum = 0;
        long previousClock = clock.Elapsed();
        long initialClock = clock.Elapsed();
        long cumTimeCost = 0, cumBytesReceived = 0, statTime = 0, cumJitter = 0;
        while (exitFlag == 0)
        {
            long bytesReceived = 0;
            while (bytesReceived < pktsize)
            {
                int ret = recv(sockfd, buffer, pktsize - bytesReceived, 0);
                if (ret == -1)
                {
                    perror("Receive failed");
                    sleep(3);
                    break;
                }
                else
                {
                    if (ret == 0)
                    {
                        printf("Client Disconnected\n");
                        exitFlag = 1;
                        break;
                    }
                    else
                    {
                        bytesReceived += ret;
                    }
                }
            }
            if (exitFlag == 1)
                break;
            cumBytesReceived += bytesReceived;

            // Handle time
            long currentClock = clock.Elapsed();
            double timeCost = currentClock - previousClock;
            previousClock = currentClock;
            cumTimeCost += timeCost;

            // Calculate
            packetNum++;
            double averageTime = 0;
            if (cumTimeCost > 0)
                averageTime = cumTimeCost / packetNum;
            cumJitter += timeCost - averageTime;

            // Stats
            statTime += timeCost;

            if (statTime >= stat)
            {
                long currentPacket = getSequence(buffer);
                double throughput = (double)(cumBytesReceived * 8) / (cumTimeCost * 1000);
                double jitter = (double)cumJitter / packetNum;
                printf("Receiver: [Elapsed] %ld ms, [Pkts] %d, [Lost] %ld, %.2f%%, [Rate] %.2f Mbps, [Jitter] %.6f ms\n", currentClock - initialClock, packetNum, currentPacket - packetNum, (double)100 * (currentPacket - packetNum) / currentPacket, throughput, jitter);
                statTime = 0;
            }
        }
    }
    close(sockfd);
    return 0;
}
