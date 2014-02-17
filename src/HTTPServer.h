/**
	httpserver
	HTTPServer.h
	Copyright 2011-2014 Ramsey Kant

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

#include <unordered_map>
#include <vector>
#include <string>

#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#ifdef __linux__
#include <kqueue/sys/event.h> // libkqueue Linux
#else
#include <sys/event.h> // kqueue BSD / OS X
#endif

#include "Client.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "ResourceHost.h"

#define INVALID_SOCKET -1
#define QUEUE_SIZE 1024

class HTTPServer {	
	// Server Socket
	int listenPort;
    int listenSocket; // Descriptor for the listening socket
    struct sockaddr_in serverAddr; // Structure for the server address

    // Kqueue
    struct timespec kqTimeout = {2, 0}; // Block for 2 seconds and 0ns at the most
	int kqfd; // kqueue descriptor
	struct kevent evList[QUEUE_SIZE]; // Events that have triggered a filter in the kqueue (max QUEUE_SIZE at a time)

	// Client map, maps Socket descriptor to Client object
    std::unordered_map<int, Client*> clientMap;

	// Resources / File System
	std::vector<ResourceHost*> hostList; // Contains all ResourceHosts
	std::unordered_map<std::string, ResourceHost*> vhosts; // Virtual hosts. Maps a host string to a ResourceHost to service the request
    
    // Connection processing
    void updateEvent(int ident, short filter, u_short flags, u_int fflags, int data, void *udata);
    void acceptConnection();
	Client *getClient(int clfd);
    void disconnectClient(Client* cl, bool mapErase = true);
    void readClient(Client* cl, int data_len); // Client read event
    bool writeClient(Client* cl, int avail_bytes); // Client write event
    
    // Request handling
    void handleRequest(Client* cl, HTTPRequest* req);
	void handleGet(Client* cl, HTTPRequest* req, ResourceHost* resHost);
	void handleOptions(Client* cl, HTTPRequest* req);
	void handleTrace(Client* cl, HTTPRequest* req);

	// Response
	void sendStatusResponse(Client* cl, int status, std::string msg = "");
	void sendResponse(Client* cl, HTTPResponse* resp, bool disconnect);
    
public:
	bool canRun;

public:
    HTTPServer();
    ~HTTPServer();

	bool start(int port);
	void stop();

	// Main event loop
	void process();
};

#endif
