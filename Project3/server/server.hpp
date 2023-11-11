#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "es_timer.hpp"
#include "thread.hpp"
#include "pipe.h"

#define MAX_CONN 10
#define TIMEOUT_SECONDS 10
#define THREADS 8

using namespace std;

class ThreadPool
{
public:
    ThreadPool(size_t numThreads) : stop(false)
    {
        pipe_t *p = pipe_new(sizeof(int), 0);

        pipe_producer_t *pros[THREADS] = {pipe_producer_new(p)};
        pipe_consumer_t *cons[THREADS] = {pipe_consumer_new(p)};

        pipe_free(p);
    }

    ~ThreadPool()
    {
        for (int i = 0; i < THREADS; i++)
        {
            pipe_producer_free(pros[i]);
            pipe_consumer_free(cons[i]);
        }
    }

private:
    pipe_t *p = pipe_new(sizeof(int), 0);

    pipe_producer_t *pros[THREADS];
    pipe_consumer_t *cons[THREADS];

    mutex queueMutex;
    bool stop;
};

int initTCP(int lport)
{
    int sockfd;
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
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
    return sockfd;
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
    int sockfd = initTCP(lport);

    // Accept TCP connection to receive settings
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
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
        if (bytesRead == 0)
        {
            cout << "Client disconnected." << endl;
        }
        else if (bytesRead < 0)
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
        data.lport = lport;
        data.client_addr = client_addr;

        // Create Thread to handle this connection
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