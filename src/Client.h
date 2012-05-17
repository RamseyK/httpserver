/**
	httpserver
	Client.h
	Copyright 2011-2012 Ramsey Kant

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	    http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

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
