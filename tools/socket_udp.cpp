// Client side implementation of UDP client-server model 
#include "socket_udp.h"

#define MAXLINE 1024

using namespace gcc_demo;

// Driver code 
void SocketUdp::InitUdpClient(std::string ip_addr, int port) { 
    // Creating socket file descriptor 
    if ( (sockfd_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    }
  
    memset(&servaddr_, 0, sizeof(servaddr_)); 
      
    // Filling server information 
    servaddr_.sin_family = AF_INET; 
    servaddr_.sin_port = htons(port); 
    servaddr_.sin_addr.s_addr = inet_addr(ip_addr.c_str()); 
}

void SocketUdp::InitUdpServer(std::string ip_addr, int port) { 
    // Creating socket file descriptor 
    if ( (sockfd_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    }
      
    memset(&servaddr_, 0, sizeof(servaddr_)); 
      
    // Filling server information 
    servaddr_.sin_family    = AF_INET; // IPv4 
    servaddr_.sin_addr.s_addr = INADDR_ANY; 
    servaddr_.sin_port = htons(PORT); 
      
    // Bind the socket with the server address 
    if (bind(sockfd_, (const struct sockaddr *)&servaddr_,  
            sizeof(servaddr_)) < 0)
    {
        perror("bind failed"); 
        exit(EXIT_FAILURE);
    }
}

bool SocketUdp::SendData(std::string& data, int len) {
  
    sendto(sockfd_, (const char *)(data.c_str()), len, 0, (const struct sockaddr *) &servaddr_,  
            sizeof(servaddr_));
    //std::cout << "[SocketUdp::SendData] sendto data" << std::endl;
    return true; 
}

bool SocketUdp::CloseSocket() {
    close(sockfd_);
    return true;
}

bool SocketUdp::RecvData(std::string& data, int& len) {
    char buffer[MAXLINE];
    len = recvfrom(sockfd_, (char *)buffer, MAXLINE,  
                0, (struct sockaddr *) &servaddr_, 
                &client_addr_len_); 
    //std::cout << "[SocketUdp::RecvData] recvdata" << std::endl;
    buffer[len] = '\0';
    std::string data_tmp(buffer, len);
    data.swap(data_tmp);
    return true;
}