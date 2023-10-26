#include <iostream>
#include <cstdlib>
#include <cstring>
#include "send.hpp"
#include "recv.hpp"
#include "host.hpp"

int main(int argc, char *argv[])
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

   std::exit(0);
}
