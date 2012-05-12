#ifndef CLIENT_H_
#define CLIENT_H_

#include <netinet/in.h>
#include <arpa/inet.h>

#define SOCKET int

class Client {
    SOCKET socketDesc; // Socket Descriptor
    sockaddr_in clientAddr;
    
public:
    Client(SOCKET fd, sockaddr_in addr);
    ~Client();
    
    sockaddr_in getClientAddr() {
        return clientAddr;
    }

    SOCKET getSocket() {
        return socketDesc;
    }
    
    char *getClientIP() {
        return inet_ntoa(clientAddr.sin_addr);
    }
};

#endif
