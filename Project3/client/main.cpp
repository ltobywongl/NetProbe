#include <iostream>
#include <cstdlib>
#include <cstring>
#include "send.hpp"
#include "recv.hpp"

using namespace std;

int main(int argc, char *argv[])
{
   int handleStatus = 0;
   if (argc == 1) {
      cout << "Please set the client mode" << endl;
      exit(EXIT_FAILURE);
   }
   // Handle Mode
   if (strcmp(argv[1], "-send") == 0)
   {
      handleStatus = handleSend(argc, argv);
   }
   else if (strcmp(argv[1], "-recv") == 0)
   {
      handleStatus = handleRecv(argc, argv);
   }

   exit(0);
}
