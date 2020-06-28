#include "receiver.h"

namespace gcc_demo {

void Receiver::Init() {
    pthread_mutex_init(&mutex_, NULL);
}

void Receiver::SendData(std::string& data, int len) {
    socket_udp_->SendData(data, len);
}

void Receiver::SendData(gcc_demo::Marshallable& m, int uri) {
    std::string output = "";
    switch (uri) {
        case ProtoUri::PROTO_NORMAL_PACKET_URI : {
           // Packet* p = dynamic_cast<Packet*>(&m);
           // PaddingString(p->data, this->GetSendPacketSize() - PROTO_HEAD_LEN - sizeof(uint32_t) - sizeof(uint64_t));
            break;
        }
        case ProtoUri::PROTO_LOSS_RATE_URI : {
            break;
        }
    }
    if (!ProtoMarshal(uri, m, output)) {
        std::cout << "marshal fail" << std::endl;
        return ;
    }
    this->SendData(output, output.size());
}

void Receiver::RecvData() {
    std::string recv_data;
    int len;
    uint32_t uri = 0;
    std::string data;
    while (true) {
        socket_udp_->RecvData(recv_data, len);
        if (!ProtoUnmarshal(recv_data, uri, data)) {
            std::cout << "proto unmarshal failed" << std::endl;
            return;
        }
        switch (uri) {
            case ProtoUri::PROTO_NORMAL_PACKET_URI : {
                    gcc_demo::Packet packet;
                    if (!PacketUnmarshal(data, packet)) {
                        std::cout << "Packet unmarshal failed" << std::endl;
                        return;
                    }
                    HandleDataPacket(packet);
                    break;
                }
            case ProtoUri::PROTO_NOT_ACK_URI : {
                    gcc_demo::PNak p_not_ack;
                    if (!PacketUnmarshal(data, p_not_ack)) {
                        std::cout << "PNak unmarshal failed" << std::endl;
                        return;
                    }
                    HandlePNack(p_not_ack);
                    break;
            }
            case ProtoUri::PROTO_LOSS_RATE_URI : {
                    gcc_demo::PLossRate ploss_rate;
                    if (!PacketUnmarshal(data, ploss_rate)) {
                        std::cout << "PLossRate unmarshal failed" << std::endl;
                        return;
                    }
                    HandlePLossRate(ploss_rate);
                    break;
            }
            case ProtoUri::PROTO_PROBE_URI : {
                    gcc_demo::Probe probe;
                    if (!PacketUnmarshal(data, probe)) {
                        std::cout << "Probe unmarshal failed" << std::endl;
                        return;
                    }
                    HandleProbePacket(probe);
                    break;
            }
        }
    }
}

void Receiver::HandleDataPacket(gcc_demo::Packet& packet) {
    
    DelNak(packet.seqid);

    gcc_demo::PacketArrivalInfo  packet_arrival_info(packet.seqid, packet.send_timestamp_ms, GetCurTimeMs());

     std::cout << "[HandleDataPacket]" << " seqid:" << packet_arrival_info.seqid << " sendtime_ms:" 
              << packet_arrival_info.send_timestamp_ms << " receivetime_ms:" << packet_arrival_info.recv_timestamp_ms << std::endl;

    arrival_packets_info_.push_back(packet_arrival_info);
    uint64_t transmit_delta_ms_ = isBiggerUint64(packet_arrival_info.recv_timestamp_ms, packet_arrival_info.send_timestamp_ms) ? 
                packet_arrival_info.recv_timestamp_ms - packet_arrival_info.send_timestamp_ms : 0;
    if (!transmit_delta_ms_)
    send_recv_timestamp_ms_.push_back(std::make_pair(packet_arrival_info.send_timestamp_ms, packet_arrival_info.recv_timestamp_ms));
    
    loss_rate_stat_.num_send_pkg++;
    if (current_max_seqid_ == -1) {
        current_max_seqid_ = packet.seqid;
        return;
    }
    int seqid = current_max_seqid_ + 1;
    while (packet.seqid > seqid) {
        seqids_not_ack_.insert(seqid++);
    }

}

void Receiver::HandlePNack(gcc_demo::PNak& seqids_not_ack) {
    seqids_not_ack_.insert(seqids_not_ack.seqids.begin(), seqids_not_ack.seqids.end());
}

void Receiver::Timer() {
    while (true) {
        CalculateReceivingBitrate();
        PRecvBitrate recv_bit_rate_packet(receiving_bitrate_);
        SendData(recv_bit_rate_packet, recv_bit_rate_packet.uri);
        sleep(timer_sec_);
    }
}

void Receiver::HandleProbePacket(gcc_demo::Probe& probe) {
    probe.recv_timestamp_ms_1 = GetCurTimeMs();
    probe.send_timestamp_ms_2 = GetCurTimeMs();
    SendData(probe, probe.uri);
    pthread_mutex_lock(&mutex_);
    probes_.push_back(probe);
    pthread_mutex_unlock(&mutex_);
}


void Receiver::DelNak(uint32_t seqid) {
    auto it = seqids_not_ack_.find(seqid);
    if (it != seqids_not_ack_.end()) {
        seqids_not_ack_.erase(it);
    }
}

void Receiver::HandlePLossRate(gcc_demo::PLossRate& ploss_rate) {
    std::cout << "[HandlePLossRate] loss_rate:" << ploss_rate.loss_rate << std::endl;
}

void Receiver::CalculateReceivingBitrate() {
    int total_recv_size = 0;    
    int64_t now = GetCurTimeMs();
    pthread_mutex_lock(&mutex_);
    for (auto iter = probes_.begin(); iter != probes_.end();) {
        if(iter->recv_timestamp_ms_1 + receiving_bitrate_timer_ > now) {
            total_recv_size += sizeof(Probe);
            iter ++;
        }
        else {
            iter = probes_.erase(iter);
        }
    }
    receiving_bitrate_ = (double)total_recv_size * 8 / (double) receiving_bitrate_timer_;
    std::cout << "bitrate:" << receiving_bitrate_ << " bps probes_size:" << probes_.size() << std::endl;
    pthread_mutex_unlock(&mutex_);
    
}


}