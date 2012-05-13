#include <iostream>
#include <string>
#include <signal.h>

#include "HTTPServer.h"

HTTPServer* svr;

// Ignore signals with this function
void handleSigPipe(int snum) {
    return;
}

// Termination signal handler
void handleTermSig(int snum) {
	svr->stop();
	delete svr;
	exit(0);
}

int main (int argc, const char * argv[])
{
    // Ignore SIGPIPE "Broken pipe" signals when socket connections are broken.
    signal(SIGPIPE, handleSigPipe);

	// Register termination signals
	signal(SIGABRT, &handleTermSig);
	signal(SIGINT, &handleTermSig);
	signal(SIGTERM, &handleTermSig);

    // Instance and start the server
	svr = new HTTPServer();
	svr->start(8080);
	
	// Input loop
	std::string input;
	while(true) {
		cin >> input;
		
		if(input == "quit") {
			break;
		} else if(input == "help") {
			cout << "come back jack!" << endl;
		} else {
			cout << "Unknown command: " << input << endl;
		}
	}
	
	// Stop the server thread
	svr->stop();
	delete svr;
    
    return 0;
}


