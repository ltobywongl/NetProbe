#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "send.h"

int main(int argc, char **argv)
{
   int handleStatus = 0;
   // Handle Mode
   if (strcmp(argv[1], "-send") == 0)
   {
      handleStatus = handleSend(argc, argv);
   }
   else if (strcmp(argv[1], "-recv") == 0)
   {
      // TODO: handleRecv
   }
   else if (strcmp(argv[1], "-host") == 0)
   {
      // TODO: handleHost
   }

   exit(0);
}

