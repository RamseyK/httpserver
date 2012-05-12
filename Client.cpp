#include "Client.h"
#include "HTTPServer.h"

Client::Client(SOCKET fd, sockaddr_in addr) {
    socketDesc = fd;
    clientAddr = addr;
}

Client::~Client() {
    
}
