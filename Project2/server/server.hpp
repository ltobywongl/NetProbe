#include <iostream>
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
#define MAX_CONN 10
#define TIMEOUT_SECONDS 10

using namespace std;

struct ThreadData
{
    int params;
    int sockfd;
    int pktrate;
    int bufsize;
};

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
    long seq = strtol(sequence, &p, 10);

    if (seq == 0) cout << message << endl;
    return seq;
}

void *handleConnection(void *parameter)
{
    ThreadData *data = reinterpret_cast<ThreadData *>(parameter);
    int params = data->params;
    int sockfd = data->sockfd;
    int pktrate = data->pktrate;
    short exitFlag = 0;
    char buffer[data->bufsize];
    struct sockaddr_in udp_addr;
    socklen_t addr_len = sizeof(udp_addr);

    cout << "Thread Received: " << params << " " << sockfd << " " << pktrate << endl;

    if (params >= 10)
    {
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(struct sockaddr_in));
        socklen_t client_len = sizeof(client_addr);

        int udpsockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (udpsockfd == -1)
        {
            perror("UDP Socket creation failed");
            close(sockfd);
            pthread_exit(nullptr);
        }

        udp_addr.sin_family = AF_INET;
        udp_addr.sin_addr.s_addr = INADDR_ANY;
        udp_addr.sin_port = 0; // Let system choose the port
        if (bind(udpsockfd, (struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0)
        {
            perror("UDP bind failed");
            close(udpsockfd);
            close(sockfd);
            pthread_exit(nullptr);
        }

        // Get the chosen port number
        getsockname(udpsockfd, (struct sockaddr *)&udp_addr, &addr_len);

        // Sending back the port num to client
        char msg[8];
        sprintf(msg, "%d", ntohs(udp_addr.sin_port));
        send(sockfd, msg, strlen(msg), 0);
        cout << "Sent UDP port number to client: " << msg << endl;

        while (exitFlag == 0)
        {
            struct timeval timeout;
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;

            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(udpsockfd, &fds);
            FD_SET(sockfd, &fds);

            int maxfd = max(udpsockfd, sockfd) + 1;
            int select_ret = select(maxfd, &fds, nullptr, nullptr, &timeout);
            
            // cout << "select_ret = " << select_ret << ", udpsockfd: " << FD_ISSET(udpsockfd, &fds) << ", tcpsockfd: " << FD_ISSET(sockfd, &fds) << endl;
            if (select_ret == -1)
            {
                perror("Select failed");
                exitFlag = 1;
                break;
            }

            if (FD_ISSET(udpsockfd, &fds))
            {
                int ret = recvfrom(udpsockfd, buffer, data->bufsize - 1, 0, (struct sockaddr *)&client_addr, &client_len);
                if (ret == -1)
                {
                    perror("Receive failed");
                    exitFlag = 1;
                    break;
                }
                cout << udpsockfd << ": " << getSequence(buffer) << endl;
                if (exitFlag == 1)
                    break;
            }

            if (FD_ISSET(sockfd, &fds))
            {
                // TCP socket check
                cout << "TCP socket check." << endl;
                int ret = recv(sockfd, buffer, data->bufsize, 0);
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
                cout << "TCP check passed " << buffer << endl;
            } else {
                // TCP socket error check
                int option = 0;
                socklen_t option_len = sizeof(option);
                if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &option, &option_len) == -1)
                {
                    perror("Get socket option failed");
                    exitFlag = 1;
                    break;
                }

                if (option != 0)
                {
                    cout << "Client disconnected" << endl;
                    exitFlag = 1;
                    break;
                }
            }
        }

        cout << "Closing UDP Socket..." << endl;
        close(udpsockfd);
    }
    else
    {
        if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &data->bufsize, sizeof(data->bufsize)) == -1) {
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
                cout << sockfd << ": " << getSequence(buffer) << endl;
            }
            if (exitFlag == 1)
                break;
        }
    }

    cout << "Closing TCP Socket..." << endl;
    close(sockfd);

    cout << "Closing Thread..." << endl;
    pthread_exit(nullptr);
}

int handleServer(int argc, char *argv[])
{
    // ** Handle Parameters **
    // Default values
    int stat = 500;
    in_addr_t lhost = INADDR_ANY;
    int lport = 4180;
    int sbufsize = 65536;
    int rbufsize = 65536;
    char *p;

    // Read options
    for (int i = 1; i < argc; i++)
    {
        if (i + 1 < argc && argv[i][0] == '-')
        {
            if (strcmp(argv[i], "-lhost") == 0)
            {
                lhost = (in_addr_t)strtol(argv[i + 1], &p, 10);
                i++;
            }
            else if (strcmp(argv[i], "-lport") == 0)
            {
                lport = strtol(argv[i + 1], &p, 10);
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
                printf("Unknown option: %s\n", argv[i]);
                return -1;
            }
        }
    }

    // ** Handle Socket **
    int sockfd, newsockfd;
    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    pthread_t thread_id;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(lport);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    cout << "Binding local socket to port number " << lport << " with late binding ... successful." << endl;

    if (listen(sockfd, MAX_CONN) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    cout << "Listening to incoming connection request ... " << endl;

    // Accept TCP connection to receive settings
    while (true)
    {
        socklen_t client_addr_len = sizeof(client_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (newsockfd < 0)
        {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        char buffer[16];
        memset(buffer, 0, sizeof(buffer));

        // Receive data from the client
        int bytesRead = recv(newsockfd, buffer, 16, 0);
        if (bytesRead > 0)
        {
            cout << "Received data from client: " << buffer << endl;
        }
        else if (bytesRead == 0)
        {
            cout << "Client disconnected." << endl;
        }
        else
        {
            perror("Receive failed");
        }

        char params[3];
        params[2] = '\0';
        strncpy(params, buffer, 2);
        int pktrate = strtol((buffer + 2), &p, 10);

        if (!(strcmp(params, "10") == 0 || strcmp(params, "01") == 0 || strcmp(params, "00") == 0 || strcmp(params, "11") == 0))
        {
            cout << "Wrong parameter format" << endl;
            close(newsockfd);
            continue;
        }

        ThreadData data;
        data.params = strtol(params, &p, 10);
        data.sockfd = newsockfd;
        data.pktrate = pktrate;
        data.bufsize = (data.params % 10) ? rbufsize : sbufsize;

        pthread_t client_thread;
        int create_thread = pthread_create(&client_thread, nullptr, handleConnection, &data);
        if (create_thread != 0)
        {
            cout << "Failed to create thread" << endl;
            close(newsockfd);
            continue;
        }
    }

    close(sockfd);

    return 0;
}