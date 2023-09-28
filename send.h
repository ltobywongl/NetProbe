#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int handleSend(int argc, char **argv)
{
   int stat = 500;
   char *rhost = "localhost";
   char *rport = "4180";
   char *proto = "UDP";
   int pktsize = 1000;
   int pktrate = 1000;
   int pktnum = 0;
   int sbufsize = 64000;
   char *p;

   // Read options
   for (int i = 2; i < argc; i++)
   {
      if (strcmp(argv[i], "-stat") == 0)
      {
         stat = strtol(argv[i + 1], &p, 10);
         i++;
      }
      else if (strcmp(argv[i], "-rhost") == 0)
      {
         rhost = argv[i + 1];
         i++;
      }
      else if (strcmp(argv[i], "-rport") == 0)
      {
         rport = argv[i + 1];
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
      else if (strcmp(argv[i], "-pktrate") == 0)
      {
         pktrate = strtol(argv[i + 1], &p, 10);
         i++;
      }
      else if (strcmp(argv[i], "-pktnum") == 0)
      {
         pktnum = strtol(argv[i + 1], &p, 10);
         i++;
      }
      else if (strcmp(argv[i], "-sbufsize") == 0)
      {
         sbufsize = strtol(argv[i + 1], &p, 10);
         i++;
      }
      else
      {
         fprintf(stderr, "Unknown option: %s\n", argv[i]);
         return -1;
      }
   }

}