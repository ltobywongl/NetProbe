#include <iostream>
#include <cstdlib>
#include <cstring>
#include "server.hpp"

using namespace std;

int main(int argc, char *argv[])
{
   int handleStatus = 0;
   handleStatus = handleServer(argc, argv);
   exit(handleStatus);
}
