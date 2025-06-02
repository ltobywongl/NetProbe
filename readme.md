## Build
### Linux Server & Client
- g++ ./Project4/server/main.cpp -o ./Project4/server/NetProbeServer -pthread
- g++ ./Project4/client/main.cpp -o ./Project4/client/NetProbeClient -lssl -lcrypto
## Run
- ./Project4/server/NetProbeServer
- ./Project4/client/NetProbeClient -send -proto TCP -pktrate 10000 [...]
