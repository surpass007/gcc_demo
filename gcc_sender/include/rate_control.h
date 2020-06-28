# pragma  once

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <string>
#include <iostream>
#include <deque>
#include <vector>

#include "sender.h"

namespace gcc_demo {

class Sender;

enum FiniteStateMachine {
    Decrease = 0,
    Hold     = 1,
    Increase = 2
};


class RateControl {
    public:
        RateControl(double estimate_target_bandwidth = 1024 * 1024);
        void Init();
        void RegisterAr(int Ar);
        int GetTargetBandwidth() {return estimate_target_bandwidth_;}
        void UpdateTargetBandwidth();
        void BitrateControl();
        void Update(BandwidthUsage bandwidth_usage);
        void Increase();
        void Decrease();
        void Hold();
        void SetRecvBitrate(double receiving_bit_rate) {receiving_bit_rate_ = receiving_bit_rate;}
        void SetSender(Sender* sender) {sender_ = sender;}
        
    private:
        double estimate_target_bandwidth_;
        double delay_based_estimate_bandwidth_;
        FiniteStateMachine state_;
        std::vector<std::vector<int> > state_graph_;
        double receiving_bit_rate_;
        double tau_;
        double alpha_;

        std::map<int, std::string> FiniteStateMachine_str_;
        std::map<int, std::string> BandwidthUsage_str_;

        Sender* sender_;
};
} // namespace