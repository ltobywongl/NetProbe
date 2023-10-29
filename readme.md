# Project for IERG4180
# SID: 1155159363
# Name: Wong Hong To
# Report: Project1/report.txt, Project2/report.txt
## Run
- g++ ./Project2/server/main.cpp -o NetProbeServer -pthread
- g++ ./Project2/client/main.cpp -o NetProbeClient
- ./Project2/server/NetProbeServer
- ./Project2/client/NetProbeClient -send -proto TCP -pktrate 10000 [...]