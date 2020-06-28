# pragma  once

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <string>
#include <iostream>
#include <deque>
#include <vector>

#include "rate_control.h"
#include "tool_helper.h" 
#include "delay_based_estimator.h"

namespace gcc_demo {

class RateControl;
   
    constexpr int kDeltaCounterMax = 1000;
    constexpr int kMinNumDeltas = 60;
    constexpr double kMaxAdaptOffsetMs = 15.0;
    constexpr double kOverUsingTimeThreshold = 10;


struct PacketTiming {
    PacketTiming(double arrival_time_ms,
    double smoothed_delay_ms,
    double raw_delay_ms) : arrival_time_ms(arrival_time_ms),
                        smoothed_delay_ms(smoothed_delay_ms),
                        raw_delay_ms(raw_delay_ms) {}
    double arrival_time_ms;
    double smoothed_delay_ms;
    double raw_delay_ms;
};


    class TrendlineEstimator : DelayBasedEstimator {
        public:
            TrendlineEstimator();
            void Update(int64_t send_time_ms, int64_t arrival_time_ms);
            void Update(double send_delta_ms, double recv_delta_ms, int64_t send_time_ms, int64_t arrival_time_ms);
            void Init();
            void UpdateTrendline(double send_delta_ms, double recv_delta_ms, int64_t send_time_ms, int64_t arrival_time_ms);
            void UpdateEstimateCapacity();
            double LinearFitSlope(const std::deque<PacketTiming>& packets);
            void Detect(double trend, double ts_delta, int64_t now_ms);
            void UpdateThreshold(double modified_trend, int64_t now_ms);
            void SetRateControl(RateControl* rate_control) {rate_control_ = rate_control;}
            BandwidthUsage State() const;


        private:
            double trendline_estimate_capacity_;
            std::vector<double> raw_accumulate_delta_;
            std::vector<double> smooth_accumulate_delta_;
            int64_t last_send_timestamp_ms_;
            int64_t last_recv_timestamp_ms_;
            double pre_trend_;
            double current_trend_;
            
            // Parameters.
            const double smoothing_coef_;
            const double threshold_gain_;
            // Used by the existing threshold.
            int num_of_deltas_;
            // Keep the arrival times small by using the change from the first packet.
            int64_t first_arrival_time_ms_;
            // Exponential backoff filtering.
            double accumulated_delay_;
            double smoothed_delay_;
            // Linear least squares regression.
            std::deque<PacketTiming> delay_hist_;

            const double k_up_;
            const double k_down_;
            double overusing_time_threshold_;
            double threshold_;
            double prev_modified_trend_;
            int64_t last_update_ms_;
            double prev_trend_;
            double time_over_using_;
            int overuse_counter_;

            BandwidthUsage hypothesis_;

            bool enable_sort_;
            int window_size_;
            RateControl* rate_control_;
        
            std::map<int, std::string> BandwidthUsage_str_;

    };
}