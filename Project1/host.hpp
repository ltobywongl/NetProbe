#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#define MAX_HOSTNAME 128

int handleHost(int argc, char *argv[])
{
    char pRemoteHost[MAX_HOSTNAME] = "localhost"; // Assume hostname stored.
    struct hostent *pHost = NULL;

    if (argc == 3)
    {
        strcpy(pRemoteHost, argv[2]);
    } else if (argc > 3) {
        printf("Too many options provided");
        return -1;
    }

    if (strcmp(pRemoteHost, "localhost") == 0) strcpy(pRemoteHost, "127.0.0.1");
    // Step 1: Determine if the it is a hostname or an IP address in dot notation.
    unsigned long ipaddr = inet_addr(pRemoteHost);
    if (ipaddr != -1)
    { // It is an IP address, reverse lookup the hostname first.
        pHost = gethostbyaddr((char *)(&ipaddr), sizeof(ipaddr), AF_INET);
        if ((pHost != NULL) && (pHost->h_name))
        {
            strncpy(pRemoteHost, pHost->h_name, MAX_HOSTNAME);
            pRemoteHost[MAX_HOSTNAME - 1] = 0; // Guarantee null-termination
        }
    }
    // Step 2: Resolve the hostname in pRemoteHost.
    pHost = gethostbyname(pRemoteHost);
    if (pHost != NULL)
    { // Successful
        printf("Official name : %s\n", (pHost->h_name) ? (pHost->h_name) : "NA");
        char *ptr = pHost->h_aliases[0];
        int i = 0;
        while (ptr)
        {
            printf("Alias %i : %s\n", i + 1, pHost->h_aliases[i]);
            ptr = pHost->h_aliases[++i];
        }
        ptr = pHost->h_addr_list[0];
        i = 0;
        while (ptr)
        {
            printf("IP Address %i : %s\n", i + 1, inet_ntoa(*((struct in_addr *)(ptr))));
            ptr = pHost->h_addr_list[++i];
        }
    }

    return 0;
}