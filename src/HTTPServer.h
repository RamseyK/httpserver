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
    std::unordered_map<int, Client*> clientMap;

    // Resources / File System
    std::vector<ResourceHost*> hostList; // Contains all ResourceHosts
    std::unordered_map<std::string, ResourceHost*> vhosts; // Virtual hosts. Maps a host string to a ResourceHost to service the request

    // Connection processing
    void updateEvent(int ident, short filter, u_short flags, u_int fflags, int32_t data, void* udata);
    void acceptConnection();
    Client* getClient(int clfd);
    void disconnectClient(Client* cl, bool mapErase = true);
    void readClient(Client* cl, int32_t data_len); // Client read event
    bool writeClient(Client* cl, int32_t avail_bytes); // Client write event
    ResourceHost* getResourceHostForRequest(const HTTPRequest* req);

    // Request handling
    void handleRequest(Client* cl, HTTPRequest* req);
    void handleGet(Client* cl, HTTPRequest* req);
    void handleOptions(Client* cl, HTTPRequest* req);
    void handleTrace(Client* cl, HTTPRequest* req);

    // Response
    void sendStatusResponse(Client* cl, int32_t status, std::string const& msg = "");
    void sendResponse(Client* cl, HTTPResponse* resp, bool disconnect);

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
