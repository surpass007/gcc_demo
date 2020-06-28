# pragma  once

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <string>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <iostream>



#define PORT 43259

class SocketWrapper;
namespace gcc_demo{

class SocketUdp {
    public:
        void InitUdpClient(std::string ip = "127.0.0.1", int port = PORT);
        void InitUdpServer(std::string ip = "127.0.0.1", int port = PORT);
        bool SendData(std::string& data, int len);
        bool RecvData(std::string& data, int& len);
        bool CloseSocket();

    private:
        unsigned client_addr_len_; 
        struct sockaddr_in servaddr_;
        int sockfd_;
};
}