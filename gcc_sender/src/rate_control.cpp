#include "rate_control.h"

namespace gcc_demo {


RateControl::RateControl(double estimate_target_bandwidth) : tau_(1.005),
                                                             alpha_(0.9) {
    estimate_target_bandwidth_ = estimate_target_bandwidth;
    receiving_bit_rate_ = 1000;  // bps
    delay_based_estimate_bandwidth_ = 1000; // bps
    state_ = FiniteStateMachine::Hold;
    state_graph_ = {
                    {FiniteStateMachine::Hold, FiniteStateMachine::Hold, FiniteStateMachine::Decrease},
                    {FiniteStateMachine::Increase, FiniteStateMachine::Hold, FiniteStateMachine::Decrease},
                    {FiniteStateMachine::Increase, FiniteStateMachine::Hold, FiniteStateMachine::Decrease}
                 };
    FiniteStateMachine_str_[0] = "Decrease";
    FiniteStateMachine_str_[1] = "Hold";
    FiniteStateMachine_str_[2] = "Increase";
    BandwidthUsage_str_[0] = "kBwNormal";
    BandwidthUsage_str_[1] = "kBwUnderusing";
    BandwidthUsage_str_[2] = "kBwOverusing";
}

// tau in [1.005, 1.3]    alpha in [0.8, 0.95]

//            Finite State Machine
//____________|___Hold____|____Increase___|__Decrease______
//__Over-use__|_Decrease__|____Decrease___|__keep/Decrease_
//___Normal___|_Increase__|_keep/Increase_|__Hold__________
//_Under-use__|_keep/Hold_|_____Hold______|__Hold__________

void RateControl::BitrateControl() {
   // sender_->SetSendingBitrate(static_cast<uint32_t>(estimate_target_bandwidth_));
}

void RateControl::UpdateTargetBandwidth() {
    double loss_based_estimate_bandwidth = sender_->GetLossBasedEstimator()->GetTargetBandwidth();
    std::cout << "[RateControl::UpdateTargetBandwidth] loss_based_estimate_bandwidth:" << loss_based_estimate_bandwidth << " delay_based_estimate_bandwidth:" << delay_based_estimate_bandwidth_ << std::endl;

   // BitrateControl();
}


void RateControl::Update(BandwidthUsage bandwidth_usage) {
    std::cout << "old stata:" << FiniteStateMachine_str_[state_];
    state_ = static_cast<FiniteStateMachine>(state_graph_[state_][bandwidth_usage]);
    std::cout << " [RateControl::Update] BandwidthUsage:" << BandwidthUsage_str_[bandwidth_usage] << " new state:" << FiniteStateMachine_str_[state_]<<" ";
    switch (state_) {
        case FiniteStateMachine::Hold : {
            std::cout << "[RateControl::Update] HOLD bitrate:" << delay_based_estimate_bandwidth_ << std::endl;
            break;
        }
        case FiniteStateMachine::Increase : {
            std::cout << "[RateControl::Update] INCREASE bitrate:" << delay_based_estimate_bandwidth_ << std::endl;
            delay_based_estimate_bandwidth_ *= tau_;
            break;
        }
        case FiniteStateMachine::Decrease : {
            
            delay_based_estimate_bandwidth_ = alpha_ * receiving_bit_rate_;
            std::cout << "[RateControl::Update] DECREASE bitrate:" << delay_based_estimate_bandwidth_ << std::endl;
            break;
        }
    }
    UpdateTargetBandwidth();
}

void RateControl::Increase() {

}

void RateControl::Decrease() {

}

void RateControl::Hold() {

}


}