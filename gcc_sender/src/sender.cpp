#include "sender.h"

namespace gcc_demo {

void Sender::Init() {
    pthread_mutex_init(&mutex_, NULL);
}

void Sender::SendData(std::string& data, int len) {
    socket_udp_->SendData(data, len);
}

void Sender::SendData(gcc_demo::Marshallable& m, int uri, uint32_t len) {
    if (!BitRateControl(len)) {
        return;
    }
    pthread_mutex_lock(&mutex_);
    if (uri == PROTO_PROBE_URI) {
        Probe* probe = dynamic_cast<Probe*>(&m);
        probes_[probe->seqid] = *probe;
    }
    pthread_mutex_unlock(&mutex_);
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


void Sender::RecvData() {
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
                        std::cout << "Probe Packet unmarshal failed" << std::endl;
                        return;
                    }
                    HandleProbePacket(probe);
                    break;
            }
            case ProtoUri::PROTO_RECV_BITRATE_URI : {
                    gcc_demo::PRecvBitrate pRecv_bitrate;
                    if (!PacketUnmarshal(data, pRecv_bitrate)) {
                        std::cout << "Probe Packet unmarshal failed" << std::endl;
                        return;
                    }
                    HandleReveivingBitratePacket(pRecv_bitrate);
                    break;
            }
        }
    }
}

void Sender::HandleDataPacket(gcc_demo::Packet& packet) {
    gcc_demo::PacketArrivalInfo  packet_arrival_info(packet.seqid, packet.send_timestamp_ms, GetCurTimeMs());
    arrival_packets_info_.push_back(packet_arrival_info);
    double transmit_delta_ms_ = isBiggerUint64(packet_arrival_info.recv_timestamp_ms, packet_arrival_info.send_timestamp_ms) ? 
                packet_arrival_info.recv_timestamp_ms - packet_arrival_info.send_timestamp_ms : 0;
   
}

void Sender::HandlePNack(gcc_demo::PNak& seqids_not_ack) {
    seqid_not_ack_.insert(seqids_not_ack.seqids.begin(), seqids_not_ack.seqids.end());
}

void Sender::HandlePLossRate(gcc_demo::PLossRate& ploss_rate) {
    current_loss_rate_ = ploss_rate.loss_rate;
}

void Sender::HandleProbePacket(gcc_demo::Probe& probe) {
   // std::cout << "receive probe packet seqid:" << probe.seqid << std::endl;
    probe.recv_timestamp_ms_2 = GetCurTimeMs();
    double rtt = (probe.recv_timestamp_ms_1 - probe.send_timestamp_ms_1 + probe.recv_timestamp_ms_2 - probe.send_timestamp_ms_2);
    
    pthread_mutex_lock(&mutex_);
    auto it = probes_.find(probe.seqid);
    if (it != probes_.end()) {
        probes_[probe.seqid].is_received = true;
    }
    pthread_mutex_unlock(&mutex_);
    network_quality_.total_rtt_ms += rtt;
   // trendline_estimator_->Update(probe.send_timestamp_ms_1, probe.recv_timestamp_ms_1);
}

void Sender::HandleReveivingBitratePacket(gcc_demo::PRecvBitrate& pRecv_bitrate) {
    std::cout << "[Sender::HandleReveivingBitratePacket] bitrate:" << pRecv_bitrate.receiving_bitrate << std::endl;
 rate_control_->SetRecvBitrate(pRecv_bitrate.receiving_bitrate);
}

void Sender::Timer() {
    int loss_rate;
    while (true) {
        CalculateLossRate();
        loss_based_estimator_->UpdateBandwidth(current_loss_rate_);
        loss_rate = network_quality_.GetLossRate() * 100;
        std::cout << "[Timer] avg_rtt:" << network_quality_.GetAvgRttMs() << "ms" << " loss_rate:" << loss_rate << "%" 
              " " << network_quality_.GetMessage() << "\n" ;
        network_quality_.Clear();
        
        sleep(timer_sec_);
    }
}

void Sender::CalculateLossRate() {
    pthread_mutex_lock(&mutex_);
    for (auto iter = probes_.begin(); iter != probes_.end();) {
        Probe& probe = iter->second;
        if (probe.is_received) {
            network_quality_.recv_packet_num ++;
            probes_.erase(iter++);
        }
        else {
            int64_t now = GetCurTimeMs();
            if (isBiggerUint64(now, probe.send_timestamp_ms_1) && (now - probe.send_timestamp_ms_1 > 1000))
            {
                network_quality_.timeout_packet_num ++;
                probes_.erase(iter++);
            }
            else {
                iter++;
            }
        }
    }
    pthread_mutex_unlock(&mutex_);
    current_loss_rate_ = network_quality_.GetLossRate();
    std::cout << "[CalculateLossRate] current_loss_rate_:" << current_loss_rate_ << std::endl;
}

bool Sender::BitRateControl(uint32_t byte_to_send) {
    int64_t now_ms = GetCurTimeMs();
    if (last_send_timestamp_ms_ == 0) {
        last_send_timestamp_ms_ = now_ms; 
    }
    uint32_t increment = ((now_ms - last_send_timestamp_ms_) * (sending_bit_rate_ / 8000)) % avaliable_bytes_upbound_;  // B/ms
    last_send_timestamp_ms_ = now_ms;
    send_avaliable_bytes_ += increment;
    if (send_avaliable_bytes_ > byte_to_send) {
        send_avaliable_bytes_ -= byte_to_send;
        return true;
    }
    return false;
}



}




