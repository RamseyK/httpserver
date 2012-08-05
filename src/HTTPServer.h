/**
	httpserver
	HTTPServer.h
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

#ifndef _HTTPSERVER_H
#define _HTTPSERVER_H

#include <unordered_map>
#include <vector>
#include <string>

#include <time.h>
#include <sys/event.h> // kqueue
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#include "Client.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "ResourceHost.h"

#define SOCKET int
#define INVALID_SOCKET -1
#define QUEUE_SIZE 1024

class HTTPServer {
	bool canRun;
	
	// Network
    SOCKET listenSocket; // Descriptor for the listening socket
    struct sockaddr_in serverAddr; // Structure for the server address
	int kqfd; // kqueue descriptor
	struct kevent evlist[QUEUE_SIZE]; // Events that have changed (max QUEUE_SIZE at a time)
    std::unordered_map<SOCKET, Client*> clientMap; // Client map, maps Socket descriptor to Client object

	// Resources / File System
	std::vector<ResourceHost*> hostList; // Contains all ResourceHosts
	std::unordered_map<std::string, ResourceHost*> vhosts; // Virtual hosts. Maps a host string to a ResourceHost to service the request
    
    // Private methods
	void process();
    void acceptConnection();
	Client *getClient(SOCKET clfd);
	void closeSockets();
    void disconnectClient(Client* cl, bool mapErase = true);
    void handleClient(Client* cl);
	void sendStatusResponse(Client* cl, int status, std::string msg = "");
	void sendResponse(Client* cl, HTTPResponse* res, bool disconnect = false);
    
    // Request handlers
    void handleRequest(Client* cl, HTTPRequest* req);
	void handleGet(Client* cl, HTTPRequest* req, ResourceHost* resHost);
	void handleOptions(Client* cl, HTTPRequest* req);
	void handleTrace(Client* cl, HTTPRequest* req);
    
public:
    HTTPServer();
    ~HTTPServer();

	void start(int port);
	void stop();
};

#endif
