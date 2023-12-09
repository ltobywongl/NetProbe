#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "es_timer.hpp"
#include "helper.hpp"

using namespace std;

in_addr getAddressByURL(string url) {
    string pRemoteHost;
    pRemoteHost = url.c_str();
    struct hostent *pHost = NULL;

    if (pRemoteHost == "localhost") pRemoteHost = "127.0.0.1";

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
    string url;
    string file = "stdout";
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
    if (!url.empty()) {
        if (url.compare(0, 7, "http://") == 0) {
            port = 80;
            url = url.substr(7);
        } else if (url.compare(0, 8, "https://") == 0) {
            port = 433;
            url = url.substr(8);
            scheme = "https";
        } else {
            cerr << "Invalid URL, provide http scheme." << endl;
        }
        
        int portPos = url.find_last_of(':');
        if (portPos != string::npos) {
            string portStr = url.substr(portPos + 1);
            try {
                port = stoi(portStr);
                url = url.substr(0, portPos);
            } catch (const exception &e) {
                cerr << "Invalid port number: " << portStr << ", falling back to 80 or 443" << endl;
            }
        }
    } else {
        cerr << "No URL Provided" << endl;
        return 1;
    }

    // Get address
    in_addr serverAddr = getAddressByURL(url);

    cout << "Scheme: " << scheme << ", URL: " << url << ", Port: " << port << ", Address: " << inet_ntoa(serverAddr) << endl;

    if (proto == "TCP") {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            cerr << "Failed to create socket" << endl;
            return -1;
        }

        // Set up the server address structure
        struct sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);
        serverAddress.sin_addr = serverAddr;

        // Connect to the server
        if (connect(sockfd, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1) {
            cerr << "Failed to connect to the server" << endl;
            close(sockfd);
            return -1;
        }

        // Send the HTTP GET request
        string request = "GET / HTTP/1.1\r\nHost: " + url + "\r\nConnection: close\r\n\r\n";
        if (send(sockfd, request.c_str(), request.length(), 0) == -1) {
            cerr << "Failed to send HTTP request" << endl;
            close(sockfd);
            return -1;
        }

        // Receive and print the response
        char buffer[32768];
        string response;
        while (true) {
            memset(buffer, 0, sizeof(buffer));
            ssize_t bytesRead = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead <= 0) {
                break;
            }
            response += buffer;
        }

        cout << "Response: " << response << endl;
        close(sockfd);
    }

    return 0;
}
