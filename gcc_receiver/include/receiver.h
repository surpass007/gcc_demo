# pragma  once

#include <iostream>
#include <deque>
#include <map>
#include <vector>
#include <time.h>
#include <set>
#include <pthread.h>

#include "socket_udp.h"
#include "tool_helper.h"
#include "protocol.h"


namespace gcc_demo {

#define TIME_INTERVAL   5

struct LossRateStat {
    uint32_t num_send_pkg;
    uint32_t num_loss_pkg;

    LossRateStat() : num_send_pkg(0), num_loss_pkg(0){}
    double GetLossRate() {
        return double(num_send_pkg) == 0 ? 0 : double(num_loss_pkg) / double(num_send_pkg);
    }
    void clear() {
        num_send_pkg = 0;
        num_loss_pkg = 0;
    }
};


class Receiver {
    public:
        Receiver() : current_max_seqid_(-1),
               timer_sec_(1),
               stat_time_interval_(TIME_INTERVAL),
               loss_rate_(0),
               receiving_bitrate_timer_(5) {}
        void Init();
        void SendData(std::string& data, int len);
        void SendData(gcc_demo::Marshallable& m, int uri);
        void RecvData();
        void SetSocketUdp(SocketUdp* socket_udp) {socket_udp_ = socket_udp;}
        void HandleDataPacket(gcc_demo::Packet& packet);
        void HandlePNack(gcc_demo::PNak& packet);
        void HandlePLossRate(gcc_demo::PLossRate& ploss_rate);
        void HandleProbePacket(gcc_demo::Probe& probe);
        void Timer();
        void DelNak(uint32_t seqid);
        void CalculateReceivingBitrate();

    
    private:
        SocketUdp* socket_udp_;
        std::deque<gcc_demo::PacketArrivalInfo> arrival_packets_info_;
        std::vector<std::pair<uint64_t, uint64_t> > send_recv_timestamp_ms_;
        std::set<uint32_t> seqids_not_ack_;
        uint32_t timer_sec_;
        uint32_t current_max_seqid_;
        double loss_rate_;
        uint32_t stat_time_interval_;
        LossRateStat loss_rate_stat_;

        std::deque<Probe> probes_;
        pthread_mutex_t mutex_;
        double receiving_bitrate_;
        uint32_t receiving_bitrate_timer_;

};

}