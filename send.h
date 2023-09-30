#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>

int handleSend(int argc, char **argv)
{
    int stat = 500;
    char *rhost = "localhost";
    int rport = 4180;
    char *proto = "UDP";
    int pktsize = 1000;
    int pktrate = 1000;
    int pktnum = 0;
    int sbufsize = 64000;
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
            else
            {
                fprintf(stderr, "Unknown option: %s\n", argv[i]);
                return -1;
            }
        }
    }
    if (rhost == "localhost") strcpy(rhost, "127.0.0.1");

    // Handle Socket
    int sockfd;
    struct sockaddr_in server_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(rport);
    server_addr.sin_addr.s_addr = inet_addr(rhost);

    // Connect to the server
    if (connect(sockfd, (const struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    char *message = "message";

    // Send data to the server
    if (send(sockfd, message, strlen(message), 0) == -1)
    {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    printf("Message sent to the server: %s\n", message);

    // Close the socket
    close(sockfd);

    return 0;
}