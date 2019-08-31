/**
    httpserver
    Client.h
    Copyright 2011-2019 Ramsey Kant

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

#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <netinet/in.h>
#include <arpa/inet.h>
#include <queue>

#include "SendQueueItem.h"

typedef unsigned char byte;

class Client {
    int socketDesc; // Socket Descriptor
    sockaddr_in clientAddr;

    std::queue<SendQueueItem*> sendQueue;

public:
    Client(int fd, sockaddr_in addr);
    ~Client();

    sockaddr_in getClientAddr() {
        return clientAddr;
    }

    int getSocket() {
        return socketDesc;
    }

    char* getClientIP() {
        return inet_ntoa(clientAddr.sin_addr);
    }

    void addToSendQueue(SendQueueItem* item);
    unsigned int sendQueueSize();
    SendQueueItem* nextInSendQueue();
    void dequeueFromSendQueue();
    void clearSendQueue();
};

#endif
