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

void *handleConnection(void *parameter)
{
    ThreadData *data = reinterpret_cast<ThreadData *>(parameter);
    int params = data->params;
    int sockfd = data->sockfd;
    int pktrate = data->pktrate;
    short exitFlag = 0;
    char buffer[data->bufsize];
    cout << "Received: " << params << " " << sockfd << " " << pktrate << endl;

    while (exitFlag == 0)
    {
        long bytesReceived = 0;
        while (bytesReceived < data->bufsize)
        {
            int ret = recv(sockfd, buffer, data->bufsize - bytesReceived, 0);
            if (ret == -1)
            {
                perror("Receive failed");
                break;
            }
            else if (ret == 0)
            {
                printf("Client Disconnected\n");
                exitFlag = 1;
                break;
            }
            bytesReceived += ret;
            cout << buffer << endl;
        }
        if (exitFlag == 1)
            break;
    }

    cout << "Closing Socket..." << endl;
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
        int newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
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
        cout << params << " " << pktrate << endl;

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