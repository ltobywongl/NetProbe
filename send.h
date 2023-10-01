#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>

char *generateMessage(int length)
{
    char *str = malloc(length + 1);
    if (str == NULL)
    {
        printf("Error: memory allocation failed\n");
        return NULL;
    }
    memset(str, '1', length);
    str[length] = '\0';
    return str;
}

int handleSend(int argc, char **argv)
{
    int stat = 500;
    char *rhost = "localhost";
    int rport = 4180;
    char *proto = "UDP";
    int pktsize = 1000;
    int pktrate = 1000;
    int pktnum = 0;
    int sbufsize = 4096;
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
    if (strcmp(rhost, "localhost") == 0)
        rhost = "127.0.0.1";

    int sockfd;
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(rport);
    server_addr.sin_addr.s_addr = inet_addr(rhost);

    if (strcmp(proto, "UDP") == 0)
    {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd == -1)
        {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }

        // Pregenerate Message
        char *message = generateMessage(pktsize);
        printf("Message size: %d\n", pktsize);

        // While need to send
        int msgsent = 0;
        while (pktnum == 0 || msgsent < pktnum)
        {
            // Send data to the server
            int bytes_sent = 0;
            while (bytes_sent < pktsize)
            {
                int r = sendto(sockfd, message + bytes_sent, pktsize - bytes_sent, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
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
            if (pktnum != 0)
                msgsent++;
        }

        // Close the socket
        close(sockfd);

        // Free
        free(message);
    }
    else if (strcmp(proto, "TCP") == 0)
    {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1)
        {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }

        // Connect to the server
        if (connect(sockfd, (const struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
        {
            perror("Connection failed");
            exit(EXIT_FAILURE);
        }

        // Pregenerate Message
        char *message = generateMessage(pktsize);
        printf("Message size: %d\n", pktsize);

        // While need to send
        int msgsent = 0;
        while (pktnum == 0 || msgsent < pktnum)
        {
            // Send data to the server
            int bytes_sent = 0;
            while (bytes_sent < pktsize)
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
            if (pktnum != 0)
                msgsent++;
        }

        // Close the socket
        close(sockfd);

        // Free
        free(message);
    }
    else
    {
        fprintf(stderr, "Message send failed");
        exit(EXIT_FAILURE);
    }

    return 0;
}