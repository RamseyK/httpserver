#include "HTTPServer.h"

/**
 * Server Constructor
 * Initialize state and server variables
 */
HTTPServer::HTTPServer() {
    listenSocket = INVALID_SOCKET;
    memset(&serverAddr, 0, sizeof(serverAddr)); // clear the struct
    keepRunning = false;

	// Create a resource manager managing the base path ./
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
}

/**
 * Init Socket
 * Initialize the Server Socket by requesting a socket handle, binding, and going into a listening state
 *
 * @param port Port to listen on
 * @return True if initialization succeeded. False if otherwise
 */
bool HTTPServer::initSocket(int port) {    
	// Create a handle for the listening socket, TCP
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(listenSocket == INVALID_SOCKET) {
		printf("Could not create socket!\n");
        return false;
	}
    
	// Populate the server address structure
	serverAddr.sin_family = AF_INET; // Family: IP protocol
	serverAddr.sin_port = htons(port); // Set the port (convert from host to netbyte order)
	serverAddr.sin_addr.s_addr = INADDR_ANY; // Let OS intelligently select the server's host address
    
	// Bind: Assign the address to the socket
	if(bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) != 0) {
		printf("Failed to bind to the address!\n");
        return false;
	}
    
	// Listen: Put the socket in a listening state, ready to accept connections
	// Accept a backlog of the OS Maximum connections in the queue
	if(listen(listenSocket, SOMAXCONN) != 0) {
		printf("Failed to put the socket in a listening state\n");
        return false;
	}
    
    // Add the listening socket to the master set and the largest FD is now the listening socket
    FD_SET(listenSocket, &fd_master);
    fdmax = listenSocket;

	keepRunning = true;

    return true;
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

void HTTPServer::runServer(int port) {
    // Initialize the socket and put it into a listening state
    if(!initSocket(port)) {
        printf("Failed to start server.\n");
        return;
    }
    
    // Processing loop
    while(keepRunning) {
        // Copy master set into fd_read for processing
        fd_read = fd_master;
        
        // Populate read_fd set with client descriptors that are ready to be read
        int selret = select(fdmax+1, &fd_read, NULL, NULL, NULL);
        if(selret < 0) {
            //printf("select failed!");
            continue;
        }
        
        // Loop through all descriptors in the read_fd set and check to see if data needs to be processed
        for(int i = 0; i <= fdmax; i++) {
            // If i isn't within the set of descriptors to be read, skip it
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
    }
    
    // Safely shutdown the server and close all open connections and sockets
    closeSockets();
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
    printf("%s has connected\n", cl->getClientIP());
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

void HTTPServer::handleClient(Client *cl) {
    if (cl == NULL)
        return;
    
    size_t dataLen = 1300;
    char *pData = new char[dataLen];
    
    // Receive data on the wire into pData
    /* TODO: Figure out what flags need to be set */
    int flags = 0; 
    ssize_t lenRecv = recv(cl->getSocket(), pData, dataLen, flags);
    
    // Determine state of the client socket and act on it
    if(lenRecv == 0) {
        // Client closed the connection
		printf("Client[%s] has opted to close the connection\n", cl->getClientIP());
        disconnectClient(cl);
    } else if(lenRecv < 0) {
        // Something went wrong with the connection
        // TODO: check perror() for the specific error message
        disconnectClient(cl);
    } else {
        // Print the data the client sent (in ascii)
        printf("%s: \n", cl->getClientIP());
        ascii_print(pData, (int)lenRecv);
        
        // Add the packet data to a string and pass to processRequest to serve the request
        string r;
        r.append(pData);
        handleRequest(cl, r);
    }

	delete [] pData;
}

void HTTPServer::sendResponse(Client *cl, HTTPResponse *res) {
    size_t dataLen = 0;
    char *sData = NULL;
    std::string strResp;
    
    // Get the string response, allocate memory for the response
    strResp = res->generateResponse();
    dataLen = strlen(strResp.c_str());
    sData = new char[dataLen];
    
    // Send the data over the wire
    send(cl->getSocket(), sData, dataLen, 0);
    
    // Delete the allocated space for the response
    if(sData != NULL)
        delete sData;
}

void HTTPServer::handleRequest(Client *cl, string requestStr) {    
    // Create an HTTPRequest object to consume the requestStr
    HTTPRequest *req = new HTTPRequest(requestStr);
    if(req == NULL)
        return;

	HTTPResponse *res = NULL;
    
    // If there was a parse error, report it and send the appropriate error in response
    if(req->hasParseError()) {
        printf("[%s] There was an error processing the request of type: %i\n", cl->getClientIP(), req->getMethod());
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
            printf("[%s] Could not handle or determine request of type: %i\n", cl->getClientIP(), req->getMethod());
        break;
    }
    
    // Send the built response to the client
	if(res != NULL)
		sendResponse(cl, res);
    
    // Free memory consumed by req and response object
    delete req;
    if(res != NULL)
        delete res;
}

HTTPResponse* HTTPServer::handleGet(Client *cl, HTTPRequest *req) {

	return NULL;
}

HTTPResponse* HTTPServer::handleHead(Client *cl, HTTPRequest *req) {
    return NULL;
}

