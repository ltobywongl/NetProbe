#include <iostream>
#include <cstdlib>
#include <cstring>

using namespace std;

int main(int argc, char *argv[])
{
   int handleStatus = 0;
   handleStatus = handleServer(argc, argv);
   exit(handleStatus);
}

int handleServer(int argc, char *argv[])
{
   // ** Handle Parameters **
   // Default
   int stat = 500;
   in_addr_t lhost = INADDR_ANY;
   int lport = 4180;
   int sbufsize = 65536;
   int rbufsize = 65536;

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

   // ** Handle Socket
   // TODO
   return 0;
}
