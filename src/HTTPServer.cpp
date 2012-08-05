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

	// Create a resource host serving the base path ./htdocs on disk
    ResourceHost* resHost = new ResourceHost("./htdocs");
	hostList.push_back(resHost);

	// Setup the resource host serving htdocs to provide for the following vhosts:
	vhosts.insert(std::pair<std::string, ResourceHost*>("127.0.0.1:8080", resHost));
	vhosts.insert(std::pair<std::string, ResourceHost*>("192.168.1.59:8080", resHost));
}

/**
 * Server Destructor
 * Closes all active connections and deletes the clientMap
 */
HTTPServer::~HTTPServer() {
	if(listenSocket != INVALID_SOCKET)
    	closeSockets();

	// Loop through hostList and delete all ResourceHosts
	while(!hostList.empty()) {
		delete hostList.back();
		hostList.pop_back();
	}
	vhosts.clear();
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
		std::cout << "Could not create socket!" << std::endl;
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
		std::cout << "Failed to bind to the address!" << std::endl;
		return;
	}
    
	// Listen: Put the socket in a listening state, ready to accept connections
	// Accept a backlog of the OS Maximum connections in the queue
	if(listen(listenSocket, SOMAXCONN) != 0) {
		std::cout << "Failed to put the socket in a listening state" << std::endl;
		return;
	}
    
	// Setup kqueue
	kqfd = kqueue();
	if(kqfd == -1) {
		std::cout << "Could not create the kernel event queue!" << std::endl;
		return;
	}
	
	// Have kqueue watch the listen socket
	struct kevent kev;
	EV_SET(&kev, listenSocket, EVFILT_READ, EV_ADD, 0, 0, NULL); // Fills kev
	kevent(kqfd, &kev, 1, NULL, 0, NULL);

	std::cout << "Server started. Listening on port " << port << "..." << std::endl;

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

	std::cout << "Server shutdown!" << std::endl;
}

/**
 * Close Sockets
 * Disconnect and delete all clients in the client map. Shutdown the listening socket
 */
void HTTPServer::closeSockets() {
    // Close all open connections and delete Client's from memory
    for(auto& x : clientMap)
        disconnectClient(x.second, false);
    
    // Clear the map
    clientMap.clear();
    
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
    
	// Have kqueue track the new client socket (udata contains pointer to Client object)
	struct kevent kev;
	EV_SET(&kev, clfd, EVFILT_READ, EV_ADD, 0, 0, cl); // Fills kev
	kevent(kqfd, &kev, 1, NULL, 0, NULL);
    
    // Add the client object to the client map
    clientMap.insert(std::pair<int, Client*>(clfd, cl));
    
    // Print the client's IP on connect
	std::cout << "[" << cl->getClientIP() << "] connected" << std::endl;
}

/**
 * Server Process
 * Main server processing function that checks for any new connections or data to read on
 * the listening socket
 */
void HTTPServer::process() {
	int nev = 0; // Number of changed events returned by kevent
	Client* cl = NULL;
	
	while(canRun) {
		// Get a list of changed socket descriptors (if any) in evlist
		// Timeout is NULL, kevent will wait for a change before returning
		nev = kevent(kqfd, NULL, 0, evlist, QUEUE_SIZE, NULL);
		
		if(nev > 0) {
	        // Loop through only the sockets that have changed in the evlist array
	        for(int i = 0; i < nev; i++) {
	            if(evlist[i].ident == (unsigned int)listenSocket) { // A client is waiting to connect
	                acceptConnection();
	            } else { // Client descriptor has triggered an event
					cl = (Client*)evlist[i].udata;
					if(evlist[i].flags & EVFILT_READ) {
	                	handleClient(cl);
					} else if(evlist[i].flags & EV_EOF) {
						disconnectClient(cl);
					} else {
						// unhandled event
					}
	            }
	        }
		} else if(nev < 0) {
			std::cout << "kevent failed!" << std::endl; // should call errno..
		} else { // Timeout
			//usleep(100);
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
	std::unordered_map<int, Client*>::const_iterator it;
    it = clientMap.find(clfd);

	// Client wasn't found
	if(it == clientMap.end())
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
    
    // Close the socket descriptor (which will also remove from kqueue's event list on the next kevent call)
    close(cl->getSocket());
    
    // Remove the client from the clientMap
	if(mapErase)
    	clientMap.erase(cl->getSocket());
    
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
		std::cout << "[" << cl->getClientIP() << "] has opted to close the connection" << std::endl;
        disconnectClient(cl);
    } else if(lenRecv < 0) {
        // Something went wrong with the connection
        // TODO: check perror() for the specific error message
        disconnectClient(cl);
    } else {
		// Data received
		/*cout << "[" << cl->getClientIP() << "] " << lenRecv << " bytes received" << endl;
		for(unsigned int i = 0; i < lenRecv; i++) {
			cout << pData[i];
		}
		cout << endl;*/
        
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
		std::cout << "[" << cl->getClientIP() << "] There was an error processing the request of type: " << req->methodIntToStr(req->getMethod()) << std::endl;
		std::cout << req->getParseError() << std::endl;
		sendStatusResponse(cl, Status(BAD_REQUEST), req->getParseError());
		return;
    }
    
    // Retrieve the host specified in the request (Required for HTTP/1.1 compliance)
	std::string host = req->getHeaderValue("Host");
	std::unordered_map<std::string, ResourceHost*>::const_iterator it = vhosts.find(host);
	ResourceHost* resHost = it->second;
	
	// Invalid Host specified by client:
	if(it == vhosts.end() || resHost == NULL) {
		sendStatusResponse(cl, Status(BAD_REQUEST), "Invalid/No Host specified: " + host);
		return;
	}
    
    // Send the request to the correct handler function
    switch(req->getMethod()) {
        case Method(HEAD):
        case Method(GET):
            handleGet(cl, req, resHost);
            break;
		case Method(OPTIONS):
			handleOptions(cl, req);
			break;
		case Method(TRACE):
			handleTrace(cl, req);
			break;
        default:
			std::cout << cl->getClientIP() << ": Could not handle or determine request of type " << req->methodIntToStr(req->getMethod()) << std::endl;
			sendStatusResponse(cl, Status(NOT_IMPLEMENTED));
		break;
    }
}

/**
 * Handle Get or Head
 * Process a GET or HEAD request to provide the client with an appropriate response
 *
 * @param cl Client requesting the resource
 * @param req State of the request
 * @param resHost Resource host to service the request
 */
void HTTPServer::handleGet(Client* cl, HTTPRequest* req, ResourceHost* resHost) {
	/*cout << "GET for: " << req->getRequestUri() << endl;
	cout << "Headers:" << endl;
	for(int i = 0; i < req->getNumHeaders(); i++) {
		cout << req->getHeaderStr(i) << endl;
	}
	cout << endl;*/
	
	// Check if the requested resource exists
	std::string uri = req->getRequestUri();
    Resource* r = resHost->getResource(uri);
	if(r != NULL) { // Exists
		HTTPResponse* res = new HTTPResponse();
		res->setStatus(Status(OK));
		res->addHeader("Content-Type", "text/html");
		res->addHeader("Content-Length", r->getSize());
		
		// Only send a message body if it's a GET request. Never send a body for HEAD
		if(req->getMethod() == Method(GET))
			res->setData(r->getData(), r->getSize());
			
		// Check if the client prefers to close the connection
		bool dc = req->getHeaderValue("Connection").compare("close") == 0;
			
		sendResponse(cl, res, dc);
		delete res;
	} else { // Not found
		sendStatusResponse(cl, Status(NOT_FOUND));
	}
}

/**
 * Handle Options
 * Process a OPTIONS request
 * OPTIONS: Return allowed capabilties for the server (*) or a particular resource
 *
 * @param cl Client requesting the resource
 * @param req State of the request
 */
void HTTPServer::handleOptions(Client* cl, HTTPRequest* req) {
	// For now, we'll always return the capabilities of the server instead of figuring it out for each resource
	std::string allow = "HEAD, GET, OPTIONS, TRACE";
	HTTPResponse* res = new HTTPResponse();
	res->setStatus(Status(OK));
	res->addHeader("Allow", allow.c_str());
	res->addHeader("Content-Length", "0"); // Required
	sendResponse(cl, res, true);
	delete res;
}

/**
 * Handle Trace
 * Process a TRACE request
 * TRACE: send back the request as received by the server verbatim
 *
 * @param cl Client requesting the resource
 * @param req State of the request
 */
void HTTPServer::handleTrace(Client* cl, HTTPRequest *req) {
	// Get a byte array representation of the request
	unsigned int len = req->size();
	byte* buf = new byte[len];
	req->setReadPos(0); // Set the read position at the beginning since the request has already been read to the end
	req->getBytes(buf, len);
	
	// Send a response with the entire request as the body
	HTTPResponse* res = new HTTPResponse();
	res->setStatus(Status(OK));
	res->addHeader("Content-Type", "message/http");
	res->addHeader("Content-Length", len);
	res->setData(buf, len);
	sendResponse(cl, res, true);
	
	delete res;
	delete buf;
}

/**
 * Send Status Response
 * Send a predefined HTTP status code response to the client consisting of
 * only the status code and required headers, then disconnect the client
 *
 * @param cl Client to send the status code to
 * @param status Status code corresponding to the enum in HTTPMessage.h
 * @param msg An additional message to append to the body text
 */
void HTTPServer::sendStatusResponse(Client* cl, int status, std::string msg) {
	HTTPResponse* res = new HTTPResponse();
	res->setStatus(Status(status));
	
	// Body message: Reason string + additional msg	
	std::string body = res->getReason() + " " + msg;
	unsigned int slen = body.length();
	char* sdata = new char[slen];
	strncpy(sdata, body.c_str(), slen);
	
	res->addHeader("Content-Type", "text/plain");
	res->addHeader("Content-Length", slen);
	res->setData((byte*)sdata, slen);
	
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
	std::string tstr;
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
			std::cout << "[" << cl->getClientIP() << "] has disconnected." << std::endl;
			disconnectClient(cl);
			delete pData;
			return;
		}

		// Adjust byte count after a successful send
		totalSent += n;
		bytesLeft -= n;
	}
	
	/*cout << "[" << cl->getClientIP() << "] was sent " << totalSent << " bytes" << endl;
	for(unsigned int i = 0; i < totalSent; i++) {
		cout << pData[i];
	}
	cout << endl;*/
	
	if(disconnect)
		disconnectClient(cl);
	
	delete pData;
}


