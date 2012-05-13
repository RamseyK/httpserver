#include "HTTPServer.h"

/**
 * Server Constructor
 * Initialize state and server variables
 */
HTTPServer::HTTPServer() {
	canRun = false;
	thread = NULL;
	
    listenSocket = INVALID_SOCKET;
    memset(&serverAddr, 0, sizeof(serverAddr)); // clear the struct

	// Create a resource manager managing the VIRTUAL base path ./
    resMgr = new ResourceManager("./", true);
    
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

	if(thread != NULL)
		delete thread;
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
	if(bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) != 0) {
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

	// Spawn thread
	canRun = true;
	thread = new boost::thread(boost::ref(*this));
}

/**
 * Stop
 * Signal the server thread to stop running and shut down
 */
void HTTPServer::stop() {
	runMutex.lock();
	canRun = false;
	runMutex.unlock();
	
	if(thread != NULL)
		thread->join();

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
    for(it = clientMap->begin(); it != clientMap->end(); it++) {
        Client *cl = it->second;
        disconnectClient(cl);
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
void HTTPServer::operator() () {
	bool run = true;
	int sret = 0;
	
	while(run) {
		// Update the running state
		runMutex.lock();
		run = canRun;
		runMutex.unlock();
		
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
			// Yield rest of time slice to CPU
			boost::this_thread::yield();
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
 */
void HTTPServer::disconnectClient(Client *cl) {
    if(cl == NULL)
        return;
    
    // Close the socket descriptor
    close(cl->getSocket());
    
    // Remove the client from the master FD map
    FD_CLR(cl->getSocket(), &fd_master);
    
    // Remove the client from the clientMap
    clientMap->erase(cl->getSocket());
    
    // Delete the client object from memory
    delete cl;
}

// http://www.yolinux.com/TUTORIALS/Sockets.html#TIPS

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
	HTTPResponse* res = NULL;
	bool dcClient = false;
	
    // Parse the request
 	// If there's an error, report it and send the appropriate error in response
    if(!req->parse()) {
		cout << "[" << cl->getClientIP() << "] There was an error processing the request of type: " << req->methodIntToStr(req->getMethod()) << endl;
		cout << req->getParseError() << endl;
		// TODO: Send appropriate HTTP error message
		disconnectClient(cl);
		return;
    }
    
    // Send the request to the correct handler function
    switch(req->getMethod()) {
        case Method(HEAD):
            res = handleHead(cl, req);
            break;
        case Method(GET):
            res = handleGet(cl, req);
            break;
        default:
			cout << cl->getClientIP() << ": Could not handle or determine request of type " << req->methodIntToStr(req->getMethod()) << endl;
        break;
    }

	// If a response could not be built, send a 500 (internal server error)
	if(res == NULL) {
		res = new HTTPResponse();
		res->setStatus(Status(SERVER_ERROR));
		std::string body = res->getStatusStr();
		res->setData((byte*)body.c_str(), body.size());
		dcClient = true;
	}
	
	// Send the built response to the client
	sendResponse(cl, res, dcClient);
	
	delete res;
}

/**
 * Handle Get
 * Process a GET request to provide the client with an appropriate response
 *
 * @param cl Client requesting the resource
 * @param req State of the request
 */
HTTPResponse* HTTPServer::handleGet(Client *cl, HTTPRequest *req) {
	HTTPResponse* res = NULL;
	
	// Check if the requested resource exists
	std::string uri = req->getRequestUri();
    Resource* r = resMgr->getResource(uri);
	if(r != NULL) { // Exists
		
	} else { // Not found
		res = new HTTPResponse();
		res->setStatus(Status(NOT_FOUND));
		std::string body = res->getStatusStr();
		res->setData((byte*)body.c_str(), body.size());
	}
	
	return res;
}

/**
 * Handle Head
 * Process a HEAD request to provide the client with an appropriate response
 *
 * @param cl Client requesting the resource
 * @param req State of the request
 */
HTTPResponse* HTTPServer::handleHead(Client *cl, HTTPRequest *req) {
	HTTPResponse* res = NULL;
	
    return NULL;
}

/**
 * Send Response
 * Send HTTPResponse packet data to a particular Client
 *
 * @param cl Client to send data to
 * @param buf ByteBuffer containing data to be sent
 * @param disconnect Should the server disconnect the client after sending (Optional, default = false)
 */
void HTTPServer::sendResponse(Client* cl, HTTPResponse* res, bool disconnect) {
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
	
	if(disconnect)
		disconnectClient(cl);
}


