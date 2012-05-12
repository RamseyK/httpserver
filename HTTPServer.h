#ifndef _HTTPSERVER_H
#define _HTTPSERVER_H

#include <map>
#include <string>

#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "Client.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "ResourceManager.h"

#define SOCKET int
#define INVALID_SOCKET -1

class HTTPServer {
    // Private variables
    SOCKET listenSocket; // Descriptor for the listening socket
    bool keepRunning; // Flag when true will keep the main loop of the server running
    struct sockaddr_in serverAddr; // Structure for the server address
    fd_set fd_master; // Master FD set (listening socket + client sockets)
    fd_set fd_read; // FD set of sockets being read / operated on
    int fdmax; // Max FD number (max sockets hanlde)
    map<SOCKET, Client*> *clientMap; // Client map, maps Socket descriptor to Client object

	ResourceManager *resMgr;
    
    // Private methods
    bool initSocket(int port = 80);
    void closeSockets();
    
public:
    HTTPServer();
    ~HTTPServer();
    void runServer(int port=80);
	void stopServer() {
		keepRunning = false;
	}
    
    void acceptConnection();
	Client *getClient(SOCKET clfd);
    void disconnectClient(Client* cl);
    void handleClient(Client* cl);
	void sendResponse(Client* cl, HTTPResponse* res, bool disconnect = false);
    
    // Request handlers
    void handleRequest(Client* cl, HTTPRequest* req);
    HTTPResponse* handleHead(Client* cl, HTTPRequest *req);
    HTTPResponse* handleGet(Client* cl, HTTPRequest *req);
};

#endif
