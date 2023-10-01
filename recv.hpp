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

double calculateAverageValue(int list[], int count)
{
    if (count == 0) return 0;
    long sum = 0;
    for (int i = 0; i < count; i++)
    {
        sum += list[i];
    }
    return (double)sum / count;
}

int handleRecv(int argc, char *argv[])
{
    int stat = 500;
    in_addr_t lhost = INADDR_ANY;
    int lport = 4180;
    char *proto = const_cast<char *>("UDP");
    int pktsize = 1000;
    int rbufsize = 4096;
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
            else if (strcmp(argv[i], "-lhost") == 0)
            {
                lhost = (in_addr_t)strtol(argv[i + 1], &p, 10);
                i++;
            }
            else if (strcmp(argv[i], "-lport") == 0)
            {
                lport = strtol(argv[i + 1], &p, 10);
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
            else if (strcmp(argv[i], "-rbufsize") == 0)
            {
                rbufsize = strtol(argv[i + 1], &p, 10);
                i++;
            }
            else
            {
                printf("Unknown option: %s\n", argv[i]);
                return -1;
            }
        }
    }

    int sockfd, newsockfd;
    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_len;
    char buffer[rbufsize];

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(lport);
    if (strcmp(proto, "UDP") == 0)
    {
        // Create socket
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd == -1)
        {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }

        // Bind the socket to the server address
        if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) == -1)
        {
            perror("Bind failed");
            exit(EXIT_FAILURE);
        }

        printf("Server listening on port %d...\n", lport);

        ES_FlashTimer clock;
        int packetNum = 0;
        long previousClock = clock.ElapseduSec();
        int arrivalTimeList[65536], jitterList[65536];
        long cumTimeCost = 0, cumBytesReceived = 0;
        while (1)
        {
            socklen_t addr_len = sizeof(client_addr);

            // Receive data from a client
            int ret = recvfrom(sockfd, buffer, rbufsize - 1, 0, (struct sockaddr *)&client_addr, &addr_len);
            if (ret == -1)
            {
                perror("Receive failed");
                exit(EXIT_FAILURE);
            }

            cumBytesReceived += ret;
            long currentClock = clock.ElapseduSec();
            double timeCost = currentClock - previousClock;
            previousClock = currentClock;

            arrivalTimeList[packetNum] = timeCost;
            packetNum++;
            double averageTime = calculateAverageValue(arrivalTimeList, packetNum);
            jitterList[packetNum - 1] = timeCost - averageTime;
            cumTimeCost += timeCost;

            if (cumTimeCost >= stat) {
                double throughput = (double)(cumBytesReceived * 8) / (cumTimeCost * 1000);
                double jitter = calculateAverageValue(jitterList, packetNum);
                printf("Receiver: [Elapsed] %ld ms, [Pkts] %d, [Rate] %.2f Mbps, [Jitter] %.6f ms\n", cumTimeCost, packetNum, throughput, jitter);
                packetNum = 0;
                cumTimeCost = 0;
                cumBytesReceived = 0;
            }
        }

        // Close the server socket
        close(sockfd);
    }
    else if (strcmp(proto, "TCP") == 0)
    {
        // Create socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1)
        {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }

        // Bind the socket to the server address
        if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) == -1)
        {
            perror("Bind failed");
            exit(EXIT_FAILURE);
        }

        // Listen for incoming connections
        if (listen(sockfd, 1) == -1)
        {
            perror("Listen failed");
            exit(EXIT_FAILURE);
        }

        printf("Server listening on port %d...\n", lport);

        while (1)
        {
            // Accept a connection from a client
            client_len = sizeof(client_addr);
            newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
            if (newsockfd == -1)
            {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));

            // Receive data from the client
            long byte_receive = 0;
            while (1)
            {
                int ret = recv(newsockfd, buffer, rbufsize, 0);
                if (ret < 0)
                {
                    perror("Receive failed");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    if (ret == 0)
                    {
                        printf("Client Disconnected\n");
                        break;
                    }
                    else
                    {
                        byte_receive += ret;
                    }
                }
                printf("Received %d bytes\n", ret);
            }

            // Close the client socket
            close(newsockfd);
        }

        // Close the server socket
        close(sockfd);
    }
    else
    {
        exit(EXIT_FAILURE);
    }

    return 0;
}