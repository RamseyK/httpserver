/**
    httpserver
    Client.h
    Copyright 2011-2025 Ramsey Kant

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

#include "SendQueueItem.h"

#include <memory>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <queue>

class Client {
    int32_t socketDesc; // Socket Descriptor
    sockaddr_in clientAddr;

    std::queue<std::shared_ptr<SendQueueItem>> sendQueue;

public:
    Client(int32_t fd, sockaddr_in addr);
    ~Client();
    Client& operator=(Client const&) = delete;  // Copy assignment
    Client(Client &&) = delete;  // Move
    Client& operator=(Client &&) = delete;  // Move assignment

    sockaddr_in getClientAddr() const {
        return clientAddr;
    }

    int32_t getSocket() const {
        return socketDesc;
    }

    char* getClientIP() {
        return inet_ntoa(clientAddr.sin_addr);
    }

    void addToSendQueue(std::shared_ptr<SendQueueItem> item);
    uint32_t sendQueueSize() const;
    std::shared_ptr<SendQueueItem> nextInSendQueue();
    void dequeueFromSendQueue();
    void clearSendQueue();
};

#endif
