#include <iostream>
#include <cstdlib>
#include <cstring>
#include "send.hpp"
#include "recv.hpp"
#include "response.hpp"

using namespace std;

int main(int argc, char *argv[])
{
   if (argc == 1) {
      cout << "Please set the client mode" << endl;
      exit(EXIT_FAILURE);
   }
   // Handle Mode
   if (strcmp(argv[1], "-send") == 0)
   {
      handleSend(argc, argv);
   }
   else if (strcmp(argv[1], "-recv") == 0)
   {
      handleRecv(argc, argv);
   }
   else if (strcmp(argv[1], "-response") == 0)
   {
      handleResponse(argc, argv);
   }
   else if (strcmp(argv[1], "-http") == 0)
   {
      handleResponse(argc, argv);
   }

   exit(0);
}
