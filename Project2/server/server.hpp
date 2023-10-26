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

void *handleConnection(void *socketPtr) {
    int clientSocket = *(reinterpret_cast<int *>(socketPtr));
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    // Receive data from the client
    int bytesRead = recv(clientSocket, buffer, 1023, 0);
    if (bytesRead > 0) {
        cout << "Received data from client: " << buffer << endl;
    } else if (bytesRead == 0) {
        cout << "Client disconnected." << endl;
    } else {
        perror("Receive failed");
    }

    cout << "Closing Socket..." << endl;
    close(clientSocket);

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
    while (true) {
        socklen_t client_addr_len = sizeof(client_addr);
        int newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (newsockfd < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        pthread_t client_thread;
        int create_thread = pthread_create(&client_thread, nullptr, handleConnection, &newsockfd);
        if (create_thread != 0) {
            cerr << "Failed to create thread" << endl;
            close(newsockfd);
            continue;
        }
    }

    close(sockfd);

    return 0;
}