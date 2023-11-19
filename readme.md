# Project for IERG4180
# SID: 1155159363
# Name: Wong Hong To
## Report: Project3/report.txt
## Build
### Linux Server & Client
- g++ ./Project3/server/main.cpp -o ./Project3/server/NetProbeServer -pthread
- g++ ./Project3/client/main.cpp -o ./Project3/client/NetProbeClient
### Windows Client
- g++ .\Project3\client\main.cpp -o .\Project3\client\NetProbeClient -lws2_32
## Run
- ./Project3/server/NetProbeServer
- ./Project3/client/NetProbeClient -send -proto TCP -pktrate 10000 [...]