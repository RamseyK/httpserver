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

#include <map>
#include <string>
#include <boost/thread.hpp>

#include <time.h>
#include <unistd.h>
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
#include "ResourceManager.h"

#define SOCKET int
#define INVALID_SOCKET -1

class HTTPServer {
	// Network
    SOCKET listenSocket; // Descriptor for the listening socket
    struct sockaddr_in serverAddr; // Structure for the server address
    fd_set fd_master; // Master FD set (listening socket + client sockets)
    fd_set fd_read; // FD set of sockets being read / operated on
	struct timeval timeout; // Select timeout
    int fdmax; // Max FD number (max sockets hanlde)
    map<SOCKET, Client*> *clientMap; // Client map, maps Socket descriptor to Client object

	// Resources / File System
	ResourceManager *resMgr;
	
	// Threading
	bool canRun;
	boost::thread* thread;
	boost::mutex runMutex;
    
    // Private methods
    void acceptConnection();
	Client *getClient(SOCKET clfd);
	void closeSockets();
    void disconnectClient(Client* cl, bool mapErase = true);
    void handleClient(Client* cl);
	void sendStatusResponse(Client* cl, int status);
	void sendResponse(Client* cl, HTTPResponse* res, bool disconnect = false);
    
    // Request handlers
    void handleRequest(Client* cl, HTTPRequest* req);
    void handleHead(Client* cl, HTTPRequest *req);
    void handleGet(Client* cl, HTTPRequest *req);
    
public:
    HTTPServer();
    ~HTTPServer();

	void start(int port);
	void stop();
	void operator() ();
};

#endif
