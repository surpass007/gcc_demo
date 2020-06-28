#include "trendline_estimator.h"

namespace gcc_demo {

TrendlineEstimator::TrendlineEstimator(): smoothing_coef_(0.9),
      threshold_gain_(0),
      num_of_deltas_(0),
      first_arrival_time_ms_(-1),
      accumulated_delay_(0),
      smoothed_delay_(0),
      delay_hist_(),
      k_up_(0.0087),
      k_down_(0.039),
      overusing_time_threshold_(kOverUsingTimeThreshold),
      threshold_(12.5),
      prev_modified_trend_(0),
      last_update_ms_(-1),
      prev_trend_(0.0),
      time_over_using_(-1),
      overuse_counter_(0),
      enable_sort_(false),
      window_size_(20),
      hypothesis_(BandwidthUsage::kBwNormal){
          BandwidthUsage_str_[0] = "kBwNormal";
          BandwidthUsage_str_[1] = "kBwUnderusing";
          BandwidthUsage_str_[2] = "kBwOverusing";
      }

void TrendlineEstimator::Init() {

}

void TrendlineEstimator::Update(int64_t send_time_ms, int64_t arrival_time_ms) {
    if (last_send_timestamp_ms_ == 0 || last_recv_timestamp_ms_ == 0) {
        last_send_timestamp_ms_ = send_time_ms;
        last_recv_timestamp_ms_ = arrival_time_ms;
        return;
    }
    double recv_delta_ms = arrival_time_ms - last_recv_timestamp_ms_;
    double send_delta_ms = send_time_ms - last_send_timestamp_ms_;
    Update(send_delta_ms, recv_delta_ms, send_time_ms, arrival_time_ms);

}

void TrendlineEstimator::UpdateTrendline(double send_delta_ms, double recv_delta_ms, int64_t send_time_ms, int64_t arrival_time_ms) {
    const double delta_ms =  recv_delta_ms - send_delta_ms;
    ++num_of_deltas_;
    num_of_deltas_ = std::min(num_of_deltas_, kDeltaCounterMax);
    if (first_arrival_time_ms_ == -1)
        first_arrival_time_ms_ = arrival_time_ms;

    // Exponential backoff filter.
    accumulated_delay_ += delta_ms;
    smoothed_delay_ = smoothing_coef_ * smoothed_delay_ + (1 - smoothing_coef_) * accumulated_delay_;

    // Maintain packet window
    delay_hist_.emplace_back(
      static_cast<double>(arrival_time_ms - first_arrival_time_ms_),
      smoothed_delay_, accumulated_delay_);
    if (enable_sort_) {
        for (size_t i = delay_hist_.size() - 1; i > 0 &&
             delay_hist_[i].arrival_time_ms < delay_hist_[i - 1].arrival_time_ms; --i) {
        std::swap(delay_hist_[i], delay_hist_[i - 1]);
        }
    }
    if (delay_hist_.size() > window_size_)
        delay_hist_.pop_front();

   // Simple linear regression.
   double trend = prev_trend_;
    if (delay_hist_.size() == window_size_) {
    // Update trend_ if it is possible to fit a line to the data. The delay
    // trend can be seen as an estimate of (send_rate - capacity)/capacity.
    // 0 < trend < 1   ->  the delay increases, queues are filling up
    //   trend == 0    ->  the delay does not change
    //   trend < 0     ->  the delay decreases, queues are being emptied
        double tmp = LinearFitSlope(delay_hist_);
        if (tmp != 0) {
            trend = tmp;
        }
    }
    std::cout << "[UpdateTrendline] trend:" << trend << std::endl;
    Detect(trend, send_delta_ms, arrival_time_ms);
    //rate_control_->Update(hypothesis_);
}

void TrendlineEstimator::Detect(double trend, double ts_delta, int64_t now_ms) {
    if (num_of_deltas_ < 2) {
        hypothesis_ = BandwidthUsage::kBwNormal;
        return;
    }
    const double modified_trend =
    std::min(num_of_deltas_, kMinNumDeltas) * trend * threshold_gain_;
    prev_modified_trend_ = modified_trend;
    if (modified_trend > threshold_) {
        if (time_over_using_ == -1) {
        // Initialize the timer. Assume that we've been
        // over-using half of the time since the previous
        // sample.
        time_over_using_ = ts_delta / 2;
        } else {
        // Increment timer
            time_over_using_ += ts_delta;
        }
    overuse_counter_++;
    if (time_over_using_ > overusing_time_threshold_ && overuse_counter_ > 1) {
      if (trend >= prev_trend_) {
        time_over_using_ = 0;
        overuse_counter_ = 0;
        hypothesis_ = BandwidthUsage::kBwOverusing;
      }
    }
  } else if (modified_trend < -threshold_) {
    time_over_using_ = -1;
    overuse_counter_ = 0;
    hypothesis_ = BandwidthUsage::kBwUnderusing;
  } else {
    time_over_using_ = -1;
    overuse_counter_ = 0;
    hypothesis_ = BandwidthUsage::kBwNormal;
  }
  prev_trend_ = trend;
  std::cout << "[Detect] modified_trend:" << modified_trend << " state:" << BandwidthUsage_str_[hypothesis_] << std::endl;
  //UpdateThreshold(modified_trend, now_ms);
}

void TrendlineEstimator::UpdateThreshold(double modified_trend, int64_t now_ms) {
    std::cout << "[UpdateThreshold]" << std::endl;
    if (last_update_ms_ == -1)
    last_update_ms_ = now_ms;

    if (Fabs(modified_trend) > threshold_ + kMaxAdaptOffsetMs) {
    // Avoid adapting the threshold to big latency spikes, caused e.g.,
    // by a sudden capacity drop.
    last_update_ms_ = now_ms;
    return;
    }

    const double k = Fabs(modified_trend) < threshold_ ? k_down_ : k_up_;
    const int64_t kMaxTimeDeltaMs = 100;
    int64_t time_delta_ms = std::min(now_ms - last_update_ms_, kMaxTimeDeltaMs);
    threshold_ += k * (Fabs(modified_trend) - threshold_) * time_delta_ms;
    threshold_ = SafeClamp(threshold_, 6.f, 600.f);
    last_update_ms_ = now_ms;
}



void TrendlineEstimator::UpdateEstimateCapacity() {

}

void TrendlineEstimator::Update(double send_delta_ms, double recv_delta_ms, int64_t send_time_ms, int64_t arrival_time_ms) {
    UpdateTrendline(send_delta_ms, recv_delta_ms, send_time_ms, arrival_time_ms);
}

double TrendlineEstimator::LinearFitSlope(const std::deque<PacketTiming>& packets) {
    std::cout << "[LinearFitSlope]" << std::endl;
    if (packets.size() >= 2) {
        // Compute the "center of mass".
        double sum_x = 0;
        double sum_y = 0;
        for (const auto& packet : packets) {
            sum_x += packet.arrival_time_ms;
            sum_y += packet.smoothed_delay_ms;
        }
        double x_avg = sum_x / packets.size();
        double y_avg = sum_y / packets.size();
        // Compute the slope k = \sum (x_i-x_avg)(y_i-y_avg) / \sum (x_i-x_avg)^2
        double numerator = 0;
        double denominator = 0;
        for (const auto& packet : packets) {
            double x = packet.arrival_time_ms;
            double y = packet.smoothed_delay_ms;
            numerator += (x - x_avg) * (y - y_avg);
            denominator += (x - x_avg) * (x - x_avg);
        }
        if (denominator == 0)
            return 0;
        return numerator / denominator;
    }
    return 0;
}

BandwidthUsage TrendlineEstimator::State() const {
  return hypothesis_;
}

} // namespace 
