/**
    httpserver
    HTTPServer.h
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

#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H_

#include "Client.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "ResourceHost.h"

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

#ifdef __linux__
#include <kqueue/sys/event.h> // libkqueue Linux - only works if libkqueue is compiled from Github sources
#else
#include <sys/event.h> // kqueue BSD / OS X
#endif

constexpr int32_t INVALID_SOCKET = -1;
constexpr uint32_t QUEUE_SIZE = 1024;

class HTTPServer {
    // Server Socket
    int32_t listenPort;
    int32_t listenSocket = INVALID_SOCKET; // Descriptor for the listening socket
    struct sockaddr_in serverAddr; // Structure for the server address
    int32_t dropUid; // setuid to this after bind()
    int32_t dropGid; // setgid to this after bind()

    // Kqueue
    struct timespec kqTimeout = {2, 0}; // Block for 2 seconds and 0ns at the most
    int32_t kqfd = -1; // kqueue descriptor
    struct kevent evList[QUEUE_SIZE]; // Events that have triggered a filter in the kqueue (max QUEUE_SIZE at a time)

    // Client map, maps Socket descriptor to Client object
    std::unordered_map<int, std::shared_ptr<Client>> clientMap;

    // Resources / File System
    std::vector<std::shared_ptr<ResourceHost>> hostList; // Contains all ResourceHosts
    std::unordered_map<std::string, std::shared_ptr<ResourceHost>, std::hash<std::string>, std::equal_to<>> vhosts; // Virtual hosts. Maps a host string to a ResourceHost to service the request

    // Connection processing
    void updateEvent(int32_t ident, int16_t filter, uint16_t flags, uint32_t fflags, int32_t data, void* udata);
    void acceptConnection();
    std::shared_ptr<Client> getClient(int32_t clfd);
    void disconnectClient(std::shared_ptr<Client> cl, bool mapErase = true);
    void readClient(std::shared_ptr<Client> cl, int32_t data_len); // Client read event
    bool writeClient(std::shared_ptr<Client> cl, int32_t avail_bytes); // Client write event
    std::shared_ptr<ResourceHost> getResourceHostForRequest(const HTTPRequest* const req);

    // Request handling
    void handleRequest(std::shared_ptr<Client> cl, HTTPRequest* const req);
    void handleGet(std::shared_ptr<Client> cl, const HTTPRequest* const req);
    void handleOptions(std::shared_ptr<Client> cl, const HTTPRequest* const req);
    void handleTrace(std::shared_ptr<Client> cl, HTTPRequest* const req);

    // Response
    void sendStatusResponse(std::shared_ptr<Client> cl, int32_t status, std::string const& msg = "");
    void sendResponse(std::shared_ptr<Client> cl, std::unique_ptr<HTTPResponse> resp, bool disconnect);

public:
    bool canRun = false;

public:
    HTTPServer(std::vector<std::string> const& vhost_aliases, int32_t port, std::string const& diskpath, int32_t drop_uid=0, int32_t drop_gid=0);
    ~HTTPServer();

    bool start();
    void stop();

    // Main event loop
    void process();
};

#endif
