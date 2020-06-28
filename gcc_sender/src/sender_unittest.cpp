// Client side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <iostream>
#include <thread>

#include "sender.h"
#include "protocol.h"
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
    gcc_demo::Sender sender;
    gcc_demo::TrendlineEstimator trendline_estimator;
    gcc_demo::RateControl rate_control;
    gcc_demo::LossBasedEstimator loss_based_estimator;

    socket_udp.InitUdpClient("144.34.148.162");
    sender.SetSocketUdp(&socket_udp);
    sender.SetTrendlindEstimator(&trendline_estimator);
    sender.SetLossBaseEstimator(&loss_based_estimator);
    trendline_estimator.SetRateControl(&rate_control);
    rate_control.SetSender(&sender);
    sender.Init();
    
    std::thread timer(&Sender::Timer, &sender);
    std::thread recv(&Sender::RecvData, &sender);
    uint32_t seqid = 1;
    std::string data;

    while(true) {
        gcc_demo::Probe probe;
        probe.seqid = seqid;
        probe.send_timestamp_ms_1 = GetCurTimeMs();
        sender.SendData(probe, probe.uri, sizeof(Probe));
        seqid = (seqid + 1);
    }

    timer.join();
    recv.join();


    return 0; 
}
