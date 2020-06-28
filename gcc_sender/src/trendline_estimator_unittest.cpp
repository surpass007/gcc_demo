#include "trendline_estimator.h"
#include <vector>

int main () {

    gcc_demo::TrendlineEstimator trendline_estimator;

    std::vector<int64_t> send_ts(1000);
    std::vector<int64_t> recv_ts(1000);
    int64_t initial_ts = 1589967058000;
    int trend_k = 2;
    send_ts[0] = initial_ts;
    recv_ts[0] = initial_ts * trend_k;
    for (int i = 1; i < 1000; i++) {
        send_ts[i] = send_ts[i - 1] + 20;
        recv_ts[i] = send_ts[i] * trend_k;
    }

    for (int i = 0; i < 1000; i++) {
        trendline_estimator.Update(send_ts[i], recv_ts[i]);
    }

    return 0;
}