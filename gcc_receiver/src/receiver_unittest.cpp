// Client side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <iostream>
#include <thread>

#include "receiver.h"
#include "tool_helper.h"


using namespace gcc_demo;

int main(int argc, char* argv[]) {
    if (argc > 4) std::cout << "error # of argument!!!" << std::endl;
    
    int packetCnt = 10;
    int packetSize = 100;

    if (argc == 3) {
        int packetCnt = atoi(argv[1]);
        int packetSize = atoi(argv[2]);
    }

    gcc_demo::SocketUdp socket_udp;
    gcc_demo::Receiver receiver;

    socket_udp.InitUdpServer();
    receiver.Init();
    receiver.SetSocketUdp(&socket_udp);

    std::thread timer(&Receiver::Timer, &receiver);

    std::string recv_data;
    int len;
    uint32_t uri = 0;
    std::string data;
    while (true) {
        receiver.RecvData();
    }
    std::cout << "send data finished" << std::endl;

    timer.join();

    return 0; 
}
