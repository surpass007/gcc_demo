
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


namespace gcc_demo {
    class GccConfig {
        public:
            GccConfig();
            void Init();

            uint32_t loss_based_window_size_;
            uint32_t delay_based_window_size_;
    };
}