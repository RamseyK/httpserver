#ifndef _HTTPSERVER_H
#define _HTTPSERVER_H

#include <map>
#include <string>

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
    
    // Debug
    void ascii_print(char* data, int length)
    {
        for(int i = 0; i <length; i++)
        {
            printf("%c ",(unsigned char)*(data+i));
        }
        printf("\n");
    }
    
public:
    HTTPServer();
    ~HTTPServer();
    void runServer(int port=80);
    
    void acceptConnection();
	Client *getClient(SOCKET clfd);
    void disconnectClient(Client *cl);
    void handleClient(Client *cl);
	void sendResponse(Client *cl, HTTPResponse *res);
    
    // Request handlers
    void handleRequest(Client *cl, string requestString);
    HTTPResponse* handleHead(Client *cl, HTTPRequest *req);
    HTTPResponse* handleGet(Client *cl, HTTPRequest *req);
};

#endif
