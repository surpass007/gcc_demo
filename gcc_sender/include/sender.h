# pragma  once

#include <iostream>
#include <deque>
#include <map>
#include <vector>
#include <time.h>
#include <set>
#include <sstream>
#include <algorithm>
#include <pthread.h>

#include "socket_udp.h"
#include "tool_helper.h"
#include "protocol.h"
#include "trendline_estimator.h"
#include "loss_based_estimator.h"


namespace gcc_demo {

class TrendlineEstimator;
class RateControl;

struct NetWorkQuality {
    uint32_t timeout_packet_num;
    uint32_t recv_packet_num;
    double total_rtt_ms;
    NetWorkQuality(): timeout_packet_num(0), recv_packet_num(0), total_rtt_ms(0) {}
    double GetAvgRttMs() {
        return (recv_packet_num == 0) ? 0 : total_rtt_ms / double(recv_packet_num);
    }
    double GetLossRate() {
        return ((double(timeout_packet_num) + double(recv_packet_num)) == 0) ? 0 :double(timeout_packet_num) / (double(timeout_packet_num) + double(recv_packet_num));
    }
    void Clear() {
        timeout_packet_num = 0;
        recv_packet_num = 0;
        total_rtt_ms = 0;
    }
    std::string GetMessage() {
        std::ostringstream oss;
        oss << "timeout_packet_num:" << timeout_packet_num << " recv_packet_num:" << recv_packet_num;
        return oss.str();
    }
};

class Sender {
    public:
        Sender(uint32_t sending_bit_rate = (1024 * 1024 * 2), uint32_t packet_size = 1000) : 
               sending_bit_rate_(sending_bit_rate), 
               packet_size_(packet_size),
               timer_sec_(1),
               send_avaliable_bytes_(100),
               last_send_timestamp_ms_(0),
               avaliable_bytes_upbound_(1 << 31) {NetWorkQuality network_quality_ = NetWorkQuality();}
        void Init();
        void SendData(std::string& data, int len);
        void SendData(gcc_demo::Marshallable& m, int uri, uint32_t len);
        void RecvData();
        void SetSocketUdp(SocketUdp* socket_udp) {socket_udp_ = socket_udp;}
        void SetTrendlindEstimator(TrendlineEstimator* trendline_estimator) {trendline_estimator_ = trendline_estimator;}
        uint32_t GetSendBitRate() {return sending_bit_rate_;}
        uint32_t GetSendPacketSize() {return packet_size_;}
        void HandleDataPacket(gcc_demo::Packet& packet);
        void HandlePNack(gcc_demo::PNak& packet);
        void HandlePLossRate(gcc_demo::PLossRate& ploss_rate);
        void HandleProbePacket(gcc_demo::Probe& probe);
        void HandleReveivingBitratePacket(gcc_demo::PRecvBitrate& pRecv_bitrate);
        void CalculateLossRate();
        void Timer();
        bool BitRateControl(uint32_t byte_to_send);
        void SetSendingBitrate(uint32_t sending_bitrate) {sending_bit_rate_ = sending_bitrate;}
        void SetLossBaseEstimator(LossBasedEstimator* loss_based_estimator) {loss_based_estimator_ = loss_based_estimator;}
        LossBasedEstimator* GetLossBasedEstimator() {return loss_based_estimator_;}

    
    private:
        SocketUdp* socket_udp_;
        uint32_t sending_bit_rate_;  // bit per second
        uint32_t packet_size_;     // byte
        std::deque<gcc_demo::PacketArrivalInfo> arrival_packets_info_;
        std::set<uint32_t> seqid_not_ack_;
        uint32_t timer_sec_;
       // DelayBasedEstimator* delay_based_estimator_;
        double current_loss_rate_;
        NetWorkQuality network_quality_;
        std::map<uint32_t, gcc_demo::Probe> probes_; // <seqid, probe>
        int64_t last_send_timestamp_ms_;
        uint32_t send_avaliable_bytes_;  // remain bytes that can be sent
        uint32_t avaliable_bytes_upbound_;
        pthread_mutex_t mutex_;
        TrendlineEstimator* trendline_estimator_;
        RateControl* rate_control_;
        LossBasedEstimator* loss_based_estimator_;
};

}