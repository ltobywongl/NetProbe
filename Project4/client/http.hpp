#include <iostream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "es_timer.hpp"
#include "helper.hpp"

using namespace std;

in_addr getAddressByURL(string url)
{
    string pRemoteHost;
    pRemoteHost = url.c_str();
    struct hostent *pHost = NULL;

    if (pRemoteHost == "localhost")
        pRemoteHost = "127.0.0.1";

    unsigned long ipaddr = inet_addr(pRemoteHost.c_str());
    if (ipaddr != -1)
    {
        pHost = gethostbyaddr((char *)(&ipaddr), sizeof(ipaddr), AF_INET);
        if ((pHost != NULL) && (pHost->h_name))
        {
            pRemoteHost = pHost->h_name;
        }
    }

    pHost = gethostbyname(pRemoteHost.c_str());
    if (pHost != NULL)
    {
        char *ptr = pHost->h_addr_list[0];
        return *(struct in_addr *)(ptr);
    }

    in_addr noneAddress;
    noneAddress.s_addr = INADDR_NONE;
    return noneAddress;
}

int handleHTTP(int argc, char *argv[])
{
    string url = "http://localhost";
    string path = "/";
    string file = "/dev/stdout";
    string proto = "UDP";
    string scheme = "http";
    int port = 80;

    // Read options
    for (int i = 2; i < argc; i++)
    {
        if (i + 1 < argc && argv[i][0] == '-')
        {
            if (strcmp(argv[i], "-url") == 0)
            {
                url = argv[i + 1];
                i++;
            }
            else if (strcmp(argv[i], "-file") == 0)
            {
                file = argv[i + 1];
                i++;
            }
            else if (strcmp(argv[i], "-proto") == 0)
            {
                proto = argv[i + 1];
                i++;
            }
            else
            {
                fprintf(stderr, "Unknown option: %s\n", argv[i]);
                return 1;
            }
        }
    }

    // Handling port
    if (!url.empty())
    {
        if (url.compare(0, 7, "http://") == 0)
        {
            port = 80;
            url = url.substr(7);
        }
        else if (url.compare(0, 8, "https://") == 0)
        {
            port = 443;
            url = url.substr(8);
            scheme = "https";
        }
        else
        {
            cerr << "Invalid URL, provide http scheme." << endl;
        }

        int portPos = url.find_last_of(':');
        if (portPos != string::npos)
        {
            string portStr = url.substr(portPos + 1);
            try
            {
                port = stoi(portStr);
                url = url.substr(0, portPos);
            }
            catch (const exception &e)
            {
                cerr << "Invalid port number: " << portStr << ", falling back to 80 or 443" << endl;
            }
        }

        int pathPos = url.find_first_of('/');
        if (pathPos == string::npos)
        {
            path = "/";
        }
        else
        {
            path = url.substr(pathPos);
            url = url.substr(0, pathPos);
        }
    }
    else
    {
        cerr << "No URL Provided" << endl;
        return 1;
    }

    if (proto == "UDP")
    {
        cerr << "TCP or QUIC only for http/https" << endl;
        return 1;
    }

    // Get address
    in_addr serverAddr = getAddressByURL(url);

    cout << "Scheme: " << scheme << ", URL: " << url << ", Path: " << path << ", Port: " << port << ", Address: " << inet_ntoa(serverAddr) << endl;

    if (proto == "TCP")
    {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1)
        {
            cerr << "Failed to create socket" << endl;
            return -1;
        }

        struct sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);
        serverAddress.sin_addr = serverAddr;

        if (connect(sockfd, reinterpret_cast<struct sockaddr *>(&serverAddress), sizeof(serverAddress)) == -1)
        {
            cerr << "Failed to connect to the server" << endl;
            close(sockfd);
            return -1;
        }

        string response;
        string request = "GET " + path + " HTTP/1.1\r\nHost: " + url + "\r\nConnection: close\r\n\r\n";
        if (scheme == "https")
        {
            SSL_library_init();
            SSL_load_error_strings();
            OpenSSL_add_all_algorithms();

            SSL_CTX *sslContext = SSL_CTX_new(TLS_client_method());
            if (sslContext == nullptr)
            {
                cerr << "Failed to create SSL context" << endl;
                close(sockfd);
                return -1;
            }

            SSL *ssl = SSL_new(sslContext);
            if (ssl == nullptr)
            {
                cerr << "Failed to create SSL object" << endl;
                SSL_CTX_free(sslContext);
                close(sockfd);
                return -1;
            }

            if (SSL_set_fd(ssl, sockfd) != 1)
            {
                cerr << "Failed to associate SSL object with socket" << endl;
                SSL_free(ssl);
                SSL_CTX_free(sslContext);
                close(sockfd);
                return -1;
            }

            if (SSL_connect(ssl) != 1)
            {
                cerr << "Failed to establish SSL connection" << endl;
                SSL_free(ssl);
                SSL_CTX_free(sslContext);
                close(sockfd);
                return -1;
            }

            if (SSL_write(ssl, request.c_str(), request.length()) <= 0)
            {
                cerr << "Failed to send HTTPS request" << endl;
                SSL_free(ssl);
                SSL_CTX_free(sslContext);
                close(sockfd);
                return -1;
            }

            char buffer[32768];
            while (true)
            {
                memset(buffer, 0, sizeof(buffer));
                ssize_t bytesRead = SSL_read(ssl, buffer, sizeof(buffer) - 1);
                if (bytesRead <= 0)
                {
                    break;
                }
                response += buffer;
            }

            SSL_shutdown(ssl);
            SSL_free(ssl);
            SSL_CTX_free(sslContext);
            close(sockfd);
        }
        else
        {
            if (send(sockfd, request.c_str(), request.length(), 0) == -1)
            {
                cerr << "Failed to send HTTP request" << endl;
                close(sockfd);
                return -1;
            }

            char buffer[32768];
            while (true)
            {
                memset(buffer, 0, sizeof(buffer));
                ssize_t bytesRead = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
                if (bytesRead <= 0)
                {
                    break;
                }
                response += buffer;
            }
        }

        if (file == "/dev/stdout")
        {
            cout << "Response: " << endl
                 << response << endl;
        }
        else if (file == "/dev/null")
        {
            // Do nothing
        }
        else
        {
            ofstream outputFile(file.c_str());
            if (!outputFile)
            {
                cerr << "Failed to open the output file." << endl;
                return 1;
            }

            outputFile << response;
            outputFile.close();
        }
        close(sockfd);
    }
    else if (proto == "QUIC")
    {
        // TODO
    }

    return 0;
}
