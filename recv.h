#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>

int handleSend(int argc, char **argv)
{
    int stat = 500;
    in_addr_t lhost = INADDR_ANY;
    int lport = 4180;
    char *proto = "UDP";
    int pktsize = 1000;
    int rbufsize = 64000;
    char *p;

    // Read options
    for (int i = 2; i < argc; i++)
    {
        if (strcmp(argv[i], "-stat") == 0)
        {
            stat = strtol(argv[i + 1], &p, 10);
            i++;
        }
        else if (strcmp(argv[i], "-lhost") == 0)
        {
            lhost = (in_addr_t)argv[i + 1];
            i++;
        }
        else if (strcmp(argv[i], "-lport") == 0)
        {
            lport = strtol(argv[i + 1], &p, 10);
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
        else if (strcmp(argv[i], "-rbufsize") == 0)
        {
            rbufsize = strtol(argv[i + 1], &p, 10);
            i++;
        }
        else
        {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return -1;
        }
    }

    return 0;
}