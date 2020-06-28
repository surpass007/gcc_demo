#include "loss_based_estimator.h"


namespace gcc_demo {

LossBasedEstimator::LossBasedEstimator(double init_bandwidth, double lower_bound_bandwidth, double upper_bound_bandwidth) : 
                                       target_bandwidth_(init_bandwidth), 
                                       lower_bound_bandwidth_(lower_bound_bandwidth), 
                                       upper_bound_bandwidth_(upper_bound_bandwidth) {
                                       }

void LossBasedEstimator::Init() {
    
}

void LossBasedEstimator::UpdateBandwidth(double loss_rate) {
    // We motivate the packet loss thresholds by noting that if the
    // transmission channel has a small amount of packet loss due to overuse
    // that amount will soon increase if the sender does not adjust his bitrate.
    // Therefore we will soon enough reach above the 10% threshold and adjust As_hat(i).
    // However, if the packet loss ratio does not increase, the losses are probably not 
    // related to self-inflicted congestion and therefore we should not react on them.
    if (loss_rate > 0.1) {
        target_bandwidth_ *= (1 - 0.5 * loss_rate);
        std::cout << "[LossBasedEstimator::UpdateBandwidth] decrease target_bandwidth:" << target_bandwidth_ << "bps" << std::endl;
    }
    else if (loss_rate < 0.02) {
        target_bandwidth_ *= 1.05;
        std::cout << "[LossBasedEstimator::UpdateBandwidth] Increase target_bandwidth:" << target_bandwidth_ << "bps" << std::endl;
    }
    else {
          //hold
          std::cout << "[LossBasedEstimator::UpdateBandwidth] Hold target_bandwidth:" << target_bandwidth_ << "bps" << std::endl;
    }
}

}

