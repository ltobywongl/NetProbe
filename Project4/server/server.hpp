#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <unistd.h>
#include <netinet/in.h>
#include <vector>
#include <mutex>
#include <pthread.h>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <functional>
#include "es_timer.hpp"
#include "thread.hpp"

#define MAX_CONN 10
#define TIMEOUT_SECONDS 10
#define THREADS 8

using namespace std;

class ThreadPool
{
public:
    ThreadPool(int threads) : stop(false)
    {
        numThreads = threads;
        pthread_mutex_init(&queueMutex, nullptr);
        pthread_cond_init(&condition, nullptr);

        pthread_t pool_size_thread;
        pthread_create(&pool_size_thread, nullptr, &ThreadPool::poolSizeEntry, this);

        for (int i = 0; i < numThreads; i++)
        {
            pthread_t thread;
            pthread_create(&thread, nullptr, &ThreadPool::threadEntry, this);
            workers.push_back(thread);
        }
    }

    ~ThreadPool()
    {
        pthread_mutex_lock(&queueMutex);
        stop = true;
        pthread_mutex_unlock(&queueMutex);
        pthread_cond_broadcast(&condition);

        for (pthread_t &thread : workers)
        {
            pthread_join(thread, nullptr);
        }

        pthread_mutex_destroy(&queueMutex);
        pthread_cond_destroy(&condition);
    }

    void enqueue(ThreadData data)
    {

        pthread_mutex_lock(&runningTasksMutex);
        if (numTasks == numThreads)
        {
            cout << "Doubling pool size to " << numThreads * 2 << endl;
            changePoolSize(numThreads * 2);
        }
        pthread_mutex_unlock(&runningTasksMutex);

        pthread_mutex_lock(&queueMutex);
        tasks.push(data);
        pthread_mutex_unlock(&queueMutex);
        pthread_cond_signal(&condition);
    }

    void changePoolSize(int newPoolSize)
    {
        pthread_mutex_lock(&queueMutex);

        for (int i = numThreads; i < newPoolSize; i++)
        {
            pthread_t thread;
            pthread_create(&thread, nullptr, &ThreadPool::threadEntry, this);
            workers.push_back(thread);
        }

        for (int i = numThreads; i < newPoolSize; i++)
        {
            pthread_cancel(workers[i]);
            pthread_join(workers[i], nullptr);
        }

        numThreads = newPoolSize;
        pthread_mutex_unlock(&queueMutex);
    }

private:
    int numThreads = 8;
    int numTasks = 0;
    bool stop;

    vector<pthread_t> workers;
    queue<ThreadData> tasks;

    pthread_mutex_t runningTasksMutex;
    pthread_mutex_t queueMutex;
    pthread_cond_t condition;

    static void *threadEntry(void *arg)
    {
        ThreadPool *pool = static_cast<ThreadPool *>(arg);
        pool->thread_handle();
        return nullptr;
    }

    static void *poolSizeEntry(void *arg)
    {
        ThreadPool *pool = static_cast<ThreadPool *>(arg);
        pool->pool_size_handle();
        return nullptr;
    }

    void pool_size_handle()
    {
        long timeLog = -1;
        ES_FlashTimer clock;
        while (true)
        {
            cout << "NumTasks: " << numTasks << " / NumThreads: " << numThreads << endl;
            long currentTime = clock.Elapsed();
            if (timeLog != -1 && timeLog < currentTime - 60000 && numThreads > 1)
            {
                cout << "Halfing pool size from " << numThreads << " to " << floor(numThreads / 2) << endl;
                changePoolSize(floor(numThreads / 2));
                timeLog = clock.Elapsed();
            }

            if (timeLog == -1 && numTasks < numThreads / 2)
            {
                timeLog = clock.Elapsed();
            }
            if (numTasks >= numThreads / 2)
            {
                timeLog = -1;
            }
            sleep(3);
        }
    }

    void thread_handle()
    {
        while (true)
        {
            // Change queue
            pthread_mutex_lock(&queueMutex);

            while (tasks.empty() && !stop)
            {
                pthread_cond_wait(&condition, &queueMutex);
            }

            if (stop && tasks.empty())
            {
                pthread_mutex_unlock(&queueMutex);
                break;
            }
            ThreadData data = tasks.front();
            tasks.pop();
            pthread_mutex_unlock(&queueMutex);

            // Change number of running tasks count
            pthread_mutex_lock(&runningTasksMutex);
            numTasks++;
            pthread_mutex_unlock(&runningTasksMutex);

            handleConnection(&data);

            // Change number of running tasks count
            pthread_mutex_lock(&runningTasksMutex);
            numTasks--;
            pthread_mutex_unlock(&runningTasksMutex);
        }
    }
};

int initTCP(int lport)
{
    int sockfd;
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));

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
    int lhttpport = 4080;
    int lhttpsport = 4081;
    int sbufsize = 65536;
    int rbufsize = 65536;
    int poolsize = 8;
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
            else if (strcmp(argv[i], "-lhttpport") == 0)
            {
                lhttpport = strtol(argv[i + 1], &p, 10);
                i++;
            }
            else if (strcmp(argv[i], "-lhttpsport") == 0)
            {
                lhttpsport = strtol(argv[i + 1], &p, 10);
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
            else if (strcmp(argv[i], "-poolsize") == 0)
            {
                poolsize = strtol(argv[i + 1], &p, 10);
                i++;
            }
            else
            {
                printf("Unknown option: %s\n", argv[i]);
                return -1;
            }
        }
    }

    // Thread Pool
    ThreadPool threadPool(poolsize);

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

        if (!(strcmp(params, "10") == 0 || strcmp(params, "01") == 0 ||
              strcmp(params, "00") == 0 || strcmp(params, "11") == 0 ||
              strcmp(params, "20") == 0 || strcmp(params, "21") == 0))
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

        threadPool.enqueue(data);
    }

    close(sockfd);

    return 0;
}