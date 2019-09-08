/**
	httpserver
	HTTPServer.cpp
	Copyright 2011-2019 Ramsey Kant

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
 *
 * @param vhost_aliases List of hostnames the HTTP server will respond to
 * @param port Port the vhost listens on
 * @param diskpath Path to the folder the vhost serves up
 */
HTTPServer::HTTPServer(std::vector<std::string> vhost_aliases, int port, std::string diskpath) {
	canRun = false;
	listenSocket = INVALID_SOCKET;
	listenPort = port;
	kqfd = -1;

	std::cout << "Primary port: " << port << ", disk path: " << diskpath.c_str() << std::endl;

	// Create a resource host serving the base path ./htdocs on disk
	ResourceHost* resHost = new ResourceHost(diskpath);
	hostList.push_back(resHost);

	// Always serve up localhost/127.0.0.1 (which is why we only added one ResourceHost to hostList above)
	char tmpstr[128];
	sprintf(tmpstr, "localhost:%i", listenPort);
	vhosts.insert(std::pair<std::string, ResourceHost*>(std::string(tmpstr).c_str(), resHost));
	sprintf(tmpstr, "127.0.0.1:%i", listenPort);
	vhosts.insert(std::pair<std::string, ResourceHost*>(std::string(tmpstr).c_str(), resHost));

	// Setup the resource host serving htdocs to provide for the vhost aliases
	for (std::string vh : vhost_aliases) {
		if (vh.length() >= 122) {
			std::cout << "vhost " << vh << " too long, skipping!" << std::endl;
			continue;
		}

		std::cout << "vhost: " << vh << std::endl;
		sprintf(tmpstr, "%s:%i", vh.c_str(), listenPort);
		vhosts.insert(std::pair<std::string, ResourceHost*>(std::string(tmpstr).c_str(), resHost));
	}
}

/**
 * Server Destructor
 * Removes all resources created in the constructor
 */
HTTPServer::~HTTPServer() {
	// Loop through hostList and delete all ResourceHosts
	while (!hostList.empty()) {
		ResourceHost* resHost = hostList.back();
		delete resHost;
		hostList.pop_back();
	}
	vhosts.clear();
}

/**
 * Start Server
 * Initialize the Server Socket by requesting a socket handle, binding, and going into a listening state
 *
 * @return True if initialization succeeded. False if otherwise
 */
bool HTTPServer::start() {
	canRun = false;

	// Create a handle for the listening socket, TCP
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET) {
		std::cout << "Could not create socket!" << std::endl;
		return false;
	}

	// Set socket as non blocking
	fcntl(listenSocket, F_SETFL, O_NONBLOCK);

	// Populate the server address structure
	// modify to support multiple address families (bottom): http://eradman.com/posts/kqueue-tcp.html
	memset(&serverAddr, 0, sizeof(struct sockaddr_in)); // clear the struct
	serverAddr.sin_family = AF_INET; // Family: IP protocol
	serverAddr.sin_port = htons(listenPort); // Set the port (convert from host to netbyte order)
	serverAddr.sin_addr.s_addr = INADDR_ANY; // Let OS intelligently select the server's host address

	// Bind: Assign the address to the socket
	if (bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0) {
		std::cout << "Failed to bind to the address!" << std::endl;
		return false;
	}

	// Listen: Put the socket in a listening state, ready to accept connections
	// Accept a backlog of the OS Maximum connections in the queue
	if (listen(listenSocket, SOMAXCONN) != 0) {
		std::cout << "Failed to put the socket in a listening state" << std::endl;
		return false;
	}

	// Setup kqueue
	kqfd = kqueue();
	if (kqfd == -1) {
		std::cout << "Could not create the kernel event queue!" << std::endl;
		return false;
	}

	// Have kqueue watch the listen socket
	updateEvent(listenSocket, EVFILT_READ, EV_ADD, 0, 0, NULL);

	canRun = true;
	std::cout << "Server ready. Listening on port " << listenPort << "..." << std::endl;
	return true;
}

/**
 * Stop
 * Disconnect all clients and cleanup all server resources created in start()
 */
void HTTPServer::stop() {
	canRun = false;

	if (listenSocket != INVALID_SOCKET) {
		// Close all open connections and delete Client's from memory
		for (auto& x : clientMap)
			disconnectClient(x.second, false);

		// Clear the map
		clientMap.clear();

		// Remove listening socket from kqueue
		updateEvent(listenSocket, EVFILT_READ, EV_DELETE, 0, 0, NULL);

		// Shudown the listening socket and release it to the OS
		shutdown(listenSocket, SHUT_RDWR);
		close(listenSocket);
		listenSocket = INVALID_SOCKET;
	}

	if (kqfd != -1) {
		close(kqfd);
		kqfd = -1;
	}

	std::cout << "Server shutdown!" << std::endl;
}

/**
 * Update Event
 * Update the kqueue by creating the appropriate kevent structure
 * See kqueue documentation for parameter descriptions
 */
void HTTPServer::updateEvent(int ident, short filter, u_short flags, u_int fflags, int data, void *udata) {
	struct kevent kev;
	EV_SET(&kev, ident, filter, flags, fflags, data, udata);
	kevent(kqfd, &kev, 1, NULL, 0, NULL);
}

/**
 * Server Process
 * Main server processing function that checks for any new connections or data to read on
 * the listening socket
 */
void HTTPServer::process() {
	int nev = 0; // Number of changed events returned by kevent
	Client* cl = NULL;
	struct kevent read_kev, write_kev;

	while (canRun) {
		// Get a list of changed socket descriptors with a read event triggered in evList
		// Timeout set in the header
		nev = kevent(kqfd, NULL, 0, evList, QUEUE_SIZE, &kqTimeout);

		if (nev <= 0)
			continue;

		// Loop through only the sockets that have changed in the evList array
		for (int i = 0; i < nev; i++) {
			if (evList[i].ident == (unsigned int)listenSocket) { // A client is waiting to connect
				acceptConnection();
			} else { // Client descriptor has triggered an event
				cl = getClient(evList[i].ident); // ident contains the clients socket descriptor
				if (cl == NULL) {
					std::cout << "Could not find client" << std::endl;
					continue;
				}

				// Client wants to disconnect
				if (evList[i].flags & EV_EOF) {
					disconnectClient(cl);
					continue;
				}

				// Clear kevent structures
				memset(&read_kev, 0, sizeof(struct kevent));
				memset(&write_kev, 0, sizeof(struct kevent));

				if (evList[i].filter == EVFILT_READ) {
					//std::cout << "read filter " << evList[i].data << " bytes available" << std::endl;
					// Read and process any pending data on the wire
					readClient(cl, evList[i].data); // data contains the number of bytes waiting to be read

					// Have kqueue disable tracking of READ events and enable tracking of WRITE events
					updateEvent(evList[i].ident, EVFILT_READ, EV_DISABLE, 0, 0, NULL);
					updateEvent(evList[i].ident, EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
				} else if (evList[i].filter == EVFILT_WRITE) {
					//std::cout << "write filter with " << evList[i].data << " bytes available" << std::endl;
					// Write any pending data to the client - writeClient returns true if there is additional data to send in the client queue
					if (!writeClient(cl, evList[i].data)) { // data contains number of bytes that can be written
						//std::cout << "switch back to read filter" << std::endl;
						// If theres nothing more to send, Have kqueue disable tracking of WRITE events and enable tracking of READ events
						updateEvent(evList[i].ident, EVFILT_READ, EV_ENABLE, 0, 0, NULL);
						updateEvent(evList[i].ident, EVFILT_WRITE, EV_DISABLE, 0, 0, NULL);
					}
				}
			}
		} // Event loop
	} // canRun
}

/**
 * Accept Connection
 * When a new connection is detected in runServer() this function is called. This attempts to accept the pending
 * connection, instance a Client object, and add to the client Map
 */
void HTTPServer::acceptConnection() {
	// Setup new client with prelim address info
	sockaddr_in clientAddr;
	int clientAddrLen = sizeof(clientAddr);
	int clfd = INVALID_SOCKET;

	// Accept the pending connection and retrive the client descriptor
	clfd = accept(listenSocket, (sockaddr*)&clientAddr, (socklen_t*)&clientAddrLen);
	if (clfd == INVALID_SOCKET)
		return;

	// Set socket as non blocking
	fcntl(clfd, F_SETFL, O_NONBLOCK);

	// Instance Client object
	Client *cl = new Client(clfd, clientAddr);

	// Add kqueue event to track the new client socket for READ and WRITE events
	updateEvent(clfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	updateEvent(clfd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, NULL); // Disabled initially

	// Add the client object to the client map
	clientMap.insert(std::pair<int, Client*>(clfd, cl));

	// Print the client's IP on connect
	std::cout << "[" << cl->getClientIP() << "] connected" << std::endl;
}

/**
 * Get Client
 * Lookup client based on the socket descriptor number in the clientMap
 *
 * @param clfd Client socket descriptor
 * @return Pointer to Client object if found. NULL otherwise
 */
Client* HTTPServer::getClient(int clfd) {
	auto it = clientMap.find(clfd);

	// Client wasn't found
	if (it == clientMap.end())
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
	if (cl == NULL)
		return;

	std::cout << "[" << cl->getClientIP() << "] disconnected" << std::endl;

	// Remove socket events from kqueue
	updateEvent(cl->getSocket(), EVFILT_READ, EV_DELETE, 0, 0, NULL);
	updateEvent(cl->getSocket(), EVFILT_WRITE, EV_DELETE, 0, 0, NULL);

	// Close the socket descriptor
	close(cl->getSocket());

	// Remove the client from the clientMap
	if (mapErase)
		clientMap.erase(cl->getSocket());

	// Delete the client object from memory
	delete cl;
}

/**
 * Read Client
 * Recieve data from a client that has indicated that it has data waiting. Pass recv'd data to handleRequest()
 * Also detect any errors in the state of the socket
 *
 * @param cl Pointer to Client that sent the data
 * @param data_len Number of bytes waiting to be read
 */
void HTTPServer::readClient(Client *cl, int data_len) {
	if (cl == NULL)
		return;

	// If the read filter triggered with 0 bytes of data, client may want to disconnect
	// Set data_len to the Ethernet max MTU by default
	if (data_len <= 0)
		data_len = 1400;

	HTTPRequest* req;
	char* pData = new char[data_len];

	// Receive data on the wire into pData
	/* TODO: Figure out what flags need to be set */
	int flags = 0;
	ssize_t lenRecv = recv(cl->getSocket(), pData, data_len, flags);

	// Determine state of the client socket and act on it
	if (lenRecv == 0) {
		// Client closed the connection
		std::cout << "[" << cl->getClientIP() << "] has opted to close the connection" << std::endl;
		disconnectClient(cl);
	} else if (lenRecv < 0) {
		// Something went wrong with the connection
		// TODO: check perror() for the specific error message
		disconnectClient(cl);
	} else {
		// Data received: Place the data in an HTTPRequest and pass it to handleRequest for processing
		req = new HTTPRequest((byte*)pData, lenRecv);
		handleRequest(cl, req);
		delete req;
	}

	delete [] pData;
}

/**
 * Write Client
 * Client has indicated it is read for writing. Write avail_bytes number of bytes to the socket if the send queue has an item
 *
 * @param cl Pointer to Client that sent the data
 * @param avail_bytes Number of bytes available for writing in the send buffer
 */
bool HTTPServer::writeClient(Client* cl, int avail_bytes) {
	if (cl == NULL)
		return false;

	int actual_sent = 0; // Actual number of bytes sent as returned by send()
	int attempt_sent = 0; // Bytes that we're attempting to send now
	int remaining = 0; // Size of data left to send for the item
	bool disconnect = false;
	byte* pData = NULL;

	// The amount of available bytes to write, reported by the OS, cant really be trusted...
	if (avail_bytes > 1400) {
		// If the available amount of data is greater than the Ethernet MTU, cap it
		avail_bytes = 1400;
	} else if (avail_bytes == 0) {
		// Sometimes OS reports 0 when its possible to send data - attempt to trickle data
		// OS will eventually increase avail_bytes
		avail_bytes = 64;
	}

	SendQueueItem* item = cl->nextInSendQueue();
	if (item == NULL)
		return false;

	pData = item->getData();
	remaining = item->getSize() - item->getOffset();
	disconnect = item->getDisconnect();

	if (avail_bytes >= remaining) {
		// Send buffer is bigger than we need, rest of item can be sent
		attempt_sent = remaining;
	} else {
		// Send buffer is smaller than we need, send the amount thats available
		attempt_sent = avail_bytes;
	}

	// Send the data and increment the offset by the actual amount sent
	actual_sent = send(cl->getSocket(), pData + (item->getOffset()), attempt_sent, 0);
	if (actual_sent >= 0)
		item->setOffset(item->getOffset() + actual_sent);
	else
		disconnect = true;

	//std::cout << "[" << cl->getClientIP() << "] was sent " << actual_sent << " bytes " << std::endl;

	// SendQueueItem isnt needed anymore. Dequeue and delete
	if (item->getOffset() >= item->getSize())
		cl->dequeueFromSendQueue();

	if (disconnect) {
		disconnectClient(cl);
		return false;
	}

	return true;
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
	if (!req->parse()) {
		std::cout << "[" << cl->getClientIP() << "] There was an error processing the request of type: " << req->methodIntToStr(req->getMethod()) << std::endl;
		std::cout << req->getParseError() << std::endl;
		sendStatusResponse(cl, Status(BAD_REQUEST), req->getParseError());
		return;
	}

	std::cout << "[" << cl->getClientIP() << "] " << req->methodIntToStr(req->getMethod()) << " " << req->getRequestUri() << std::endl;
	/*std::cout << "Headers:" << std::endl;
	for(int i = 0; i < req->getNumHeaders(); i++) {
		std::cout << "\t" << req->getHeaderStr(i) << std::endl;
	}
	std::cout << std::endl;*/

	// Determine the appropriate vhost
	ResourceHost* resHost = NULL;
	std::string host = "";

	// Retrieve the host specified in the request (Required for HTTP/1.1 compliance)
	if (req->getVersion().compare(HTTP_VERSION_11) == 0) {
		host = req->getHeaderValue("Host");

		// All vhosts have the port appended, so need to append it to the host if it doesnt exist
		if (host.find(":") == std::string::npos) {
			host.append(":" + std::to_string(listenPort));
		}

		std::unordered_map<std::string, ResourceHost*>::const_iterator it = vhosts.find(host);

		if (it != vhosts.end())
			resHost = it->second;
	} else {
		// Temporary: HTTP/1.0 are given the first ResouceHost in the hostList
		// TODO: Allow admin to specify a 'default resource host'
		if (hostList.size() > 0)
			resHost = hostList[0];
	}

	// ResourceHost couldnt be determined or the Host specified by the client was invalid
	if (resHost == NULL) {
		sendStatusResponse(cl, Status(BAD_REQUEST), "Invalid/No Host specified: " + host);
		return;
	}

	// Send the request to the correct handler function
	switch (req->getMethod()) {
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
		std::cout << "[" << cl->getClientIP() << "] Could not handle or determine request of type " << req->methodIntToStr(req->getMethod()) << std::endl;
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
	// Check if the requested resource exists
	std::string uri = req->getRequestUri();
	Resource* r = resHost->getResource(uri);

	if (r != NULL) { // Exists
		std::cout << "[" << cl->getClientIP() << "] " << "Sending file: " << uri << std::endl;

		HTTPResponse* resp = new HTTPResponse();
		resp->setStatus(Status(OK));
		resp->addHeader("Content-Type", r->getMimeType());
		resp->addHeader("Content-Length", r->getSize());

		// Only send a message body if it's a GET request. Never send a body for HEAD
		if (req->getMethod() == Method(GET))
			resp->setData(r->getData(), r->getSize());

		bool dc = false;

		// HTTP/1.0 should close the connection by default
		if (req->getVersion().compare(HTTP_VERSION_10) == 0)
			dc = true;

		// If Connection: close is specified, the connection should be terminated after the request is serviced
		std::string connection_val = req->getHeaderValue("Connection");
		if (connection_val.compare("close") == 0)
			dc = true;

		sendResponse(cl, resp, dc);
		delete resp;
		delete r;
	} else { // Not found
		std::cout << "[" << cl->getClientIP() << "] " << "File not found: " << uri << std::endl;
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

	HTTPResponse* resp = new HTTPResponse();
	resp->setStatus(Status(OK));
	resp->addHeader("Allow", allow.c_str());
	resp->addHeader("Content-Length", "0"); // Required

	sendResponse(cl, resp, true);
	delete resp;
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
	HTTPResponse* resp = new HTTPResponse();
	resp->setStatus(Status(OK));
	resp->addHeader("Content-Type", "message/http");
	resp->addHeader("Content-Length", len);
	resp->setData(buf, len);
	sendResponse(cl, resp, true);

	delete resp;
	delete[] buf;
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
	HTTPResponse* resp = new HTTPResponse();
	resp->setStatus(Status(status));

	// Body message: Reason string + additional msg
	std::string body = resp->getReason() + ": " + msg;
	unsigned int slen = body.length();
	char* sdata = new char[slen];
	strncpy(sdata, body.c_str(), slen);

	resp->addHeader("Content-Type", "text/plain");
	resp->addHeader("Content-Length", slen);
	resp->setData((byte*)sdata, slen);

	sendResponse(cl, resp, true);

	delete resp;
}

/**
 * Send Response
 * Send a generic HTTPResponse packet data to a particular Client
 *
 * @param cl Client to send data to
 * @param buf ByteBuffer containing data to be sent
 * @param disconnect Should the server disconnect the client after sending (Optional, default = false)
 */
void HTTPServer::sendResponse(Client* cl, HTTPResponse* resp, bool disconnect) {
	// Server Header
	resp->addHeader("Server", "httpserver/1.0");

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
	resp->addHeader("Date", tstr);

	// Include a Connection: close header if this is the final response sent by the server
	if (disconnect)
		resp->addHeader("Connection", "close");

	// Get raw data by creating the response (we are responsible for cleaning it up in process())
	byte* pData = resp->create();

	// Add data to the Client's send queue
	cl->addToSendQueue(new SendQueueItem(pData, resp->size(), disconnect));
}


