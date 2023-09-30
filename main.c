#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "send.h"
#include "recv.h"
#include "host.h"

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
      handleStatus = handleRecv(argc, argv);
   }
   else if (strcmp(argv[1], "-host") == 0)
   {
      handleStatus = handleHost(argc, argv);
   }

   exit(0);
}

