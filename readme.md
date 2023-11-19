# Project for IERG4180
# SID: 1155159363
# Name: Wong Hong To
## Report: Project1/report.txt, Project2/report.txt, Project3/report.txt
## Run
- g++ ./Project3/server/main.cpp -o NetProbeServer -pthread
- g++ ./Project3/client/main.cpp -o NetProbeClient
- ./Project3/server/NetProbeServer
- ./Project3/client/NetProbeClient -send -proto TCP -pktrate 10000 [...]