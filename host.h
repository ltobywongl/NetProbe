#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>

int handleHost(int argc, char **argv)
{
    char pRemoteHost[MAX_HOSTNAME]; // Assume hostname stored.
    struct hostent *pHost = NULL;
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
        else if (WSAGetLastError() == WSANO_DATA)
        {
            printf("\n No DNS Record for the IP address found.");
        }
        else
        {
            printf("\n gethostbyaddr() failed with code %i\n", WSAGetLastError());
        }
    }
    // Step 2: Resolve the hostname in pRemoteHost.
    pHost = gethostbyname(pRemoteHost);
    if (pHost != NULL)
    { // Successful
        printf("\n Official name : %s", (pHost->h_name) ? (pHost->h_name) : "NA");
        char *ptr = pHost->h_aliases[0];
        int i = 0;
        while (ptr)
        {
            printf("\n Alias %i : %s", i + 1, pHost->h_aliases[i]);
            ptr = pHost->h_aliases[++i];
        }
        ptr = pHost->h_addr_list[0];
        i = 0;
        while (ptr)
        {
            printf("\n IP Address %i : %s", i + 1, inet_ntoa(*((struct in_addr *)(ptr))));
            ptr = pHost->h_addr_list[++i];
        }
    }
    // free(pHost);
}