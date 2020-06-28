# pragma  once

#include <string>
#include <map>
#include <deque>

#include "packet.h"

namespace gcc_demo {

enum  BandwidthUsage {
  kBwNormal = 0,
  kBwUnderusing = 1,
  kBwOverusing = 2,
  kLast
};

enum ServiceType
{
	SERVICE_TYPE_GVOICE = 0x0f1f << 16,
};

enum ProtoUri
{
	PROTO_NORMAL_PACKET_URI		 = SERVICE_TYPE_GVOICE | 0x1000,
	PROTO_NOT_ACK_URI            = SERVICE_TYPE_GVOICE | 0x1001,
	PROTO_LOSS_RATE_URI          = SERVICE_TYPE_GVOICE | 0x1002,
	PROTO_PROBE_URI		         = SERVICE_TYPE_GVOICE | 0x1003,
    PROTO_RECV_BITRATE_URI       = SERVICE_TYPE_GVOICE | 0x1004
};

struct Packet : public Marshallable {
    enum {
        uri = PROTO_NORMAL_PACKET_URI
    };
    uint32_t seqid;
    uint64_t send_timestamp_ms;
    std::string data;
    Packet() : 
    seqid(0), 
    send_timestamp_ms(0), 
    data("") {}
    Packet(uint64_t seqId, uint64_t timestamp_ms, std::string strdata) : 
    seqid(seqId), 
    send_timestamp_ms(timestamp_ms), 
    data(strdata){}
    virtual void marshal(Pack& pk) const {
        pk << seqid << send_timestamp_ms << data;
    }
    virtual void unmarshal(const Unpack& upk) {
        upk >> seqid >> send_timestamp_ms >> data;
    }
};

struct Probe : public Marshallable {
    enum {
        uri = PROTO_PROBE_URI
    };
    uint32_t seqid;
    uint64_t send_timestamp_ms_1;
    uint64_t recv_timestamp_ms_1;
    uint64_t send_timestamp_ms_2;
    uint64_t recv_timestamp_ms_2;
    bool is_received;
    Probe() : 
    seqid(0), 
    send_timestamp_ms_1(0), 
    recv_timestamp_ms_1(0),
    send_timestamp_ms_2(0), 
    recv_timestamp_ms_2(0),
    is_received(false) {}
    virtual void marshal(Pack& pk) const {
        pk << seqid << send_timestamp_ms_1 << recv_timestamp_ms_1 << send_timestamp_ms_2 << recv_timestamp_ms_2 << is_received;
    }
    virtual void unmarshal(const Unpack& upk) {
        upk >> seqid >> send_timestamp_ms_1 >> recv_timestamp_ms_1 >> send_timestamp_ms_2 >> recv_timestamp_ms_2 >> is_received;
    }
};

struct PNak : public Marshallable {
    enum {
        uri = PROTO_NOT_ACK_URI
    };
    std::set<uint32_t> seqids;
    virtual void marshal(Pack& pk) const {
        marshal_container(pk, seqids);
    }
    virtual void unmarshal(const Unpack& upk) {
        unmarshal_container(upk, std::inserter(seqids, seqids.begin()));
    }
};

struct PLossRate : public Marshallable {
    enum {
        uri = PROTO_LOSS_RATE_URI
    };
    uint32_t loss_rate;
    PLossRate():loss_rate(0) {}
    PLossRate(uint32_t lossrate) : loss_rate(lossrate){};
    virtual void marshal(Pack& pk) const {
        pk << loss_rate;
    }
    virtual void unmarshal(const Unpack& upk) {
        upk >> loss_rate;
    }
};

struct PRecvBitrate : public Marshallable {
    enum {
        uri = PROTO_RECV_BITRATE_URI
    };
    uint32_t receiving_bitrate;
    PRecvBitrate():receiving_bitrate(0) {}
    PRecvBitrate(uint32_t receiving_bitrate) : receiving_bitrate(receiving_bitrate){};
    virtual void marshal(Pack& pk) const {
        pk << receiving_bitrate;
    }
    virtual void unmarshal(const Unpack& upk) {
        upk >> receiving_bitrate;
    }
};

struct PacketArrivalInfo {
    uint32_t seqid;
    uint64_t send_timestamp_ms;
    uint64_t recv_timestamp_ms;
    PacketArrivalInfo() : seqid(0), send_timestamp_ms(0), recv_timestamp_ms(0) {}
    PacketArrivalInfo(uint64_t seq_id, uint64_t send_ts, uint64_t recv_ts) : seqid(seq_id), send_timestamp_ms(send_ts), recv_timestamp_ms(recv_ts){}
};

}