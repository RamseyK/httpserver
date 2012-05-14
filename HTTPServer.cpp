/**
	httpserver
	HTTPServer.cpp
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

#include "HTTPServer.h"

/**
 * Server Constructor
 * Initialize state and server variables
 */
HTTPServer::HTTPServer() {
	canRun = false;
	
    listenSocket = INVALID_SOCKET;
    memset(&serverAddr, 0, sizeof(serverAddr)); // clear the struct

	// Create a resource manager managing the VIRTUAL base path ./
    resMgr = new ResourceManager("./htdocs/");
    
    // Instance clientMap, relates Socket Descriptor to pointer to Client object
    clientMap = new map<SOCKET, Client*>();
    
    // Zero the file descriptor sets
    FD_ZERO(&fd_master);
    FD_ZERO(&fd_read);
}

/**
 * Server Destructor
 * Closes all active connections and deletes the clientMap
 */
HTTPServer::~HTTPServer() {
	if(listenSocket != INVALID_SOCKET)
    	closeSockets();
    delete clientMap;
}

/**
 * Start Server
 * Initialize the Server Socket by requesting a socket handle, binding, and going into a listening state
 *
 * @param port Port to listen on
 * @return True if initialization succeeded. False if otherwise
 */
void HTTPServer::start(int port) {    
	// Create a handle for the listening socket, TCP
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(listenSocket == INVALID_SOCKET) {
		cout << "Could not create socket!" << endl;
		return;
	}
	
	// Set socket as non blocking
	fcntl(listenSocket, F_SETFL, O_NONBLOCK);
    
	// Populate the server address structure
	serverAddr.sin_family = AF_INET; // Family: IP protocol
	serverAddr.sin_port = htons(port); // Set the port (convert from host to netbyte order)
	serverAddr.sin_addr.s_addr = INADDR_ANY; // Let OS intelligently select the server's host address
    
	// Bind: Assign the address to the socket
	if(bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0) {
		cout << "Failed to bind to the address!" << endl;
		return;
	}
    
	// Listen: Put the socket in a listening state, ready to accept connections
	// Accept a backlog of the OS Maximum connections in the queue
	if(listen(listenSocket, SOMAXCONN) != 0) {
		cout << "Failed to put the socket in a listening state" << endl;
		return;
	}
    
    // Add the listening socket to the master set and the largest FD is now the listening socket
    FD_SET(listenSocket, &fd_master);
    fdmax = listenSocket;

	// Set select to timeout at 50 microseconds
	timeout.tv_sec = 0;
	timeout.tv_usec = 50;

	cout << "Server started. Listening on port " << port << "..." << endl;

	// Start processing
	canRun = true;
	process();
}

/**
 * Stop
 * Signal the server thread to stop running and shut down
 */
void HTTPServer::stop() {
	canRun = false;

    // Safely shutdown the server and close all open connections and sockets
    closeSockets();

	cout << "Server shutdown!" << endl;
}

/**
 * Close Sockets
 * Disconnect and delete all clients in the client map. Shutdown the listening socket
 */
void HTTPServer::closeSockets() {
    // Close all open connections and delete Client's from memory
    std::map<int, Client*>::const_iterator it;
    for(it = clientMap->begin(); it != clientMap->end(); ++it) {
        Client *cl = it->second;
        disconnectClient(cl, false);
    }
    
    // Clear the map
    clientMap->clear();
    
    // Shudown the listening socket and release it to the OS
	shutdown(listenSocket, SHUT_RDWR);
	close(listenSocket);
	listenSocket = INVALID_SOCKET;
}

/**
 * Accept Connection
 * When a new connection is detected in runServer() this function is called. This attempts to accept the pending connection, instance a Client object, and add to the client Map
 */
void HTTPServer::acceptConnection() {
    // Setup new client with prelim address info
    sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    SOCKET clfd = INVALID_SOCKET;
    
    // Accept the pending connection and retrive the client descriptor
    clfd = accept(listenSocket, (sockaddr*)&clientAddr, (socklen_t*)&clientAddrLen);
    if(clfd == INVALID_SOCKET)
        return;

	// Set socket as non blocking
	fcntl(clfd, F_SETFL, O_NONBLOCK);
    
    // Instance Client object
    Client *cl = new Client(clfd, clientAddr);
    
    // Add to the master FD set
    FD_SET(clfd, &fd_master);
    
    // If the new client's handle is greater than the max, set the new max
    if(clfd > fdmax)
        fdmax = clfd;
    
    // Add the client object to the client map
    clientMap->insert(std::pair<int, Client*>(clfd, cl));
    
    // Print the client's IP on connect
	cout << "[" << cl->getClientIP() << "] connected" << endl;
}

/**
 * Server Process
 * Main server processing function that checks for any new connections or data to read on
 * the listening socket
 */
void HTTPServer::process() {
	int sret = 0;
	
	while(canRun) {
		// Copy master set into fd_read for processing
		fd_read = fd_master;

		// Populate read_fd set with client descriptors that are ready to be read
		// return values: -1 = unsuccessful, 0 = timeout, > 0 - # of sockets that need to be read
		sret = select(fdmax+1, &fd_read, NULL, NULL, &timeout);
		if(sret > 0) {
			// Loop through all descriptors in the read_fd set and check to see if data needs to be processed
			for(int i = 0; i <= fdmax; i++) {
				// Socket i isn't ready to be read (not in the read set), continue
				if(!FD_ISSET(i, &fd_read))
					continue;

				// A new client is waiting to be accepted on the listenSocket
				if(listenSocket == i) {
					acceptConnection();
				} else { // The descriptor is a client
					Client *cl = getClient(i);
					handleClient(cl);
				}
			}
		} else if(sret < 0) {
			cout << "Select failed!" << endl;
			break;
		} else { // Timeout
			usleep(100);
		}
	}
}

/**
 * Get Client
 * Lookup client based on the socket descriptor number in the clientMap
 *
 * @param clfd Client Descriptor
 * @return Pointer to Client object if found. NULL otherwise
 */
Client* HTTPServer::getClient(SOCKET clfd) {
	std::map<int, Client*>::const_iterator it;
    it = clientMap->find(clfd);

	// Client wasn't found
	if(it == clientMap->end())
		return NULL;

    // Return a pointer to the client object
    return it->second;
}

/**
 * Disconnect Client
 * Close the client's socket descriptor and release it from the FD map, client map, and memory
 *
 * @param cl Pointer to Client object
 * @param mapErase When true, remove the client from the client map. Needed if operations on the
 * client map are being performed and we don't want to remove the map entry right away
 */
void HTTPServer::disconnectClient(Client *cl, bool mapErase) {
    if(cl == NULL)
        return;
    
    // Close the socket descriptor
    close(cl->getSocket());
    
    // Remove the client from the master FD map
    FD_CLR(cl->getSocket(), &fd_master);
    
    // Remove the client from the clientMap
	if(mapErase)
    	clientMap->erase(cl->getSocket());
    
    // Delete the client object from memory
    delete cl;
}

/**
 * Handle Client
 * Recieve data from a client that has indicated (via select()) that it has data waiting. Pass recv'd data to handleRequest()
 * Also detect any errors in the state of the socket
 *
 * @param cl Pointer to Client that sent the data
 */
void HTTPServer::handleClient(Client *cl) {
    if (cl == NULL)
        return;
    
	HTTPRequest* req;
    size_t dataLen = 1300;
    char* pData = new char[dataLen];
    
    // Receive data on the wire into pData
    /* TODO: Figure out what flags need to be set */
    int flags = 0; 
    ssize_t lenRecv = recv(cl->getSocket(), pData, dataLen, flags);
    
    // Determine state of the client socket and act on it
    if(lenRecv == 0) {
        // Client closed the connection
		cout << "[" << cl->getClientIP() << "] has opted to close the connection" << endl;
        disconnectClient(cl);
    } else if(lenRecv < 0) {
        // Something went wrong with the connection
        // TODO: check perror() for the specific error message
        disconnectClient(cl);
    } else {
		// Data received
		cout << "[" << cl->getClientIP() << "] " << lenRecv << " bytes received" << endl;
        
        // Place the data in an HTTPRequest and pass it to handleRequest for processing
		req = new HTTPRequest((byte*)pData, lenRecv);
        handleRequest(cl, req);
		delete req;
    }

	delete [] pData;
}

/**
 * Handle Request
 * Process an incoming request from a Client. Send request off to appropriate handler function
 * that corresponds to an HTTP operation (GET, HEAD etc) :)
 *
 * @param cl Client object where request originated from
 * @param req HTTPRequest object filled with raw packet data
 */
void HTTPServer::handleRequest(Client *cl, HTTPRequest* req) {
    // Parse the request
 	// If there's an error, report it and send a server error in response
    if(!req->parse()) {
		cout << "[" << cl->getClientIP() << "] There was an error processing the request of type: " << req->methodIntToStr(req->getMethod()) << endl;
		cout << req->getParseError() << endl;
		sendStatusResponse(cl, Status(SERVER_ERROR));
		return;
    }
    
    // Send the request to the correct handler function
    switch(req->getMethod()) {
        case Method(HEAD):
            handleHead(cl, req);
            break;
        case Method(GET):
            handleGet(cl, req);
            break;
        default:
			cout << cl->getClientIP() << ": Could not handle or determine request of type " << req->methodIntToStr(req->getMethod()) << endl;
			sendStatusResponse(cl, Status(NOT_IMPLEMENTED));
		break;
    }
}

/**
 * Handle Get
 * Process a GET request to provide the client with an appropriate response
 *
 * @param cl Client requesting the resource
 * @param req State of the request
 */
void HTTPServer::handleGet(Client *cl, HTTPRequest *req) {
	// Check if the requested resource exists
	std::string uri = req->getRequestUri();
    Resource* r = resMgr->getResource(uri);
	if(r != NULL) { // Exists
		HTTPResponse* res = new HTTPResponse();
		res->setStatus(Status(OK));
		std::stringstream sz;
		sz << r->getSize();
		res->addHeader("Content-Type", "text/html");
		res->addHeader("Content-Length", sz.str());
		res->setData(r->getData(), r->getSize());
		sendResponse(cl, res, true);
		delete res;
	} else { // Not found
		sendStatusResponse(cl, Status(NOT_FOUND));
	}
}

/**
 * Handle Head
 * Process a HEAD request to provide the client with an appropriate response
 *
 * @param cl Client requesting the resource
 * @param req State of the request
 */
void HTTPServer::handleHead(Client *cl, HTTPRequest *req) {
	// Check if the requested resource exists
	std::string uri = req->getRequestUri();
    Resource* r = resMgr->getResource(uri);
	if(r != NULL) { // Exists
		// Only include headers associated with the file. NEVER contains a body
		HTTPResponse* res = new HTTPResponse();
		res->setStatus(Status(OK));
		std::stringstream sz;
		sz << r->getSize();
		res->addHeader("Content-Type", "text/html");
		res->addHeader("Content-Length", sz.str());
		sendResponse(cl, res, true);
		delete res;
	} else { // Not found
		sendStatusResponse(cl, Status(NOT_FOUND));
	}
}

/**
 * Send Status Response
 * Send a predefined HTTP status code response to the client consisting of
 * only the status code and required headers, then disconnect the client
 *
 * @param cl Client to send the status code to
 * @param status Status code corresponding to the enum in HTTPMessage.h
 */
void HTTPServer::sendStatusResponse(Client* cl, int status) {
	HTTPResponse* res = new HTTPResponse();
	res->setStatus(Status(status));
	std::string body = res->getReason();
	std::stringstream sz;
	sz << body.size();
	res->addHeader("Content-Type", "text/html");
	res->addHeader("Content-Length", sz.str());
	res->setData((byte*)body.c_str(), body.size());
	
	sendResponse(cl, res, true);
	
	delete res;
}

/**
 * Send Response
 * Send a generic HTTPResponse packet data to a particular Client
 *
 * @param cl Client to send data to
 * @param buf ByteBuffer containing data to be sent
 * @param disconnect Should the server disconnect the client after sending (Optional, default = false)
 */
void HTTPServer::sendResponse(Client* cl, HTTPResponse* res, bool disconnect) {
	// Server Header
	res->addHeader("Server", "httpserver/1.0");
	
	// Time stamp the response with the Date header
	string tstr;
	char tbuf[36];
	time_t rawtime;
	struct tm* ptm;
	time(&rawtime);
	ptm = gmtime(&rawtime);
	// Ex: Fri, 31 Dec 1999 23:59:59 GMT
	strftime(tbuf, 36, "%a, %d %b %Y %H:%M:%S GMT", ptm);
	tstr = tbuf;
	res->addHeader("Date", tstr);
	
	// Include a Connection: close header if this is the final response sent by the server
	if(disconnect)
		res->addHeader("Connection", "close");
	
	// Get raw data by creating the response (pData will be cleaned up by the response obj)
	byte* pData = res->create();
	
	// Retrieve sizes
	size_t totalSent = 0, bytesLeft = res->size(), dataLen = res->size();
    ssize_t n = 0;

	// Solution to deal with partials sends...loop till totalSent matches dataLen
	while(totalSent < dataLen) {
		n = send(cl->getSocket(), pData+totalSent, bytesLeft, 0);

		// Client closed the connection
		if(n < 0) {
			cout << "[" << cl->getClientIP() << "] has disconnected." << endl;
			disconnectClient(cl);
			break;
		}

		// Adjust byte count after a successful send
		totalSent += n;
		bytesLeft -= n;
	}
	
	cout << "[" << cl->getClientIP() << "] was sent " << totalSent << " bytes" << endl;
	for(unsigned int i = 0; i < totalSent; i++) {
		cout << pData[i];
	}
	cout << endl;
	
	if(disconnect)
		disconnectClient(cl);
}


