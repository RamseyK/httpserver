#include <iostream>
#include <signal.h>

#include "HTTPServer.h"
#include "Testers.h"

void handleSigPipe(int snum);
void handleSigQuit(int snum);

HTTPServer* svr;

// Ignore signals with this function
void handleSigPipe(int snum) {
    return;
}

void handleSigQuit(int snum) {
	if(svr != NULL)
		svr->stopServer();
}

int main (int argc, const char * argv[])
{
    // Ignore SIGPIPE "Broken pipe" signals when socket connections are broken.
    signal(SIGPIPE, handleSigPipe);

	// Register termination signals to stop the server
	signal(SIGABRT, &handleSigQuit);
	signal(SIGINT, &handleSigQuit);
	signal(SIGTERM, &handleSigQuit);

    // Instance and start the server
	svr = new HTTPServer();
	svr->runServer(8080);
	delete svr;

    //testAll();
    
    return 0;
}


