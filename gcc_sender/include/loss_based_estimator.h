# pragma  once

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <string>
#include <iostream>
#include <deque>

#include "socket_udp.h"
#include "tool_helper.h" 
#include "protocol.h"
#include "config.h"

namespace gcc_demo {
    class LossBasedEstimator {
        public:
            LossBasedEstimator(double init_bandwidth = 1024 * 1024, double lower_bound_bandwidth = 1024, double upper_bound_bandwidth = 1024 * 1024 * 10);
            void Init();
            void UpdateBandwidth(double loss_rate);
            double GetTargetBandwidth() {return target_bandwidth_;}
           
        private:
            double upper_bound_bandwidth_;
            double lower_bound_bandwidth_;
            double current_loss_rate_;
            double target_bandwidth_;
    };
}