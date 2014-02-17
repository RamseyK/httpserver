/**
	httpserver
	main.cpp
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

#include <iostream>
#include <string>
#include <signal.h>

#include "HTTPServer.h"

static HTTPServer* svr;

// Ignore signals with this function
void handleSigPipe(int snum) {
    return;
}

// Termination signal handler (Ctrl-C)
void handleTermSig(int snum) {
	svr->canRun = false;
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

	// Run main event loop
	svr->process();
	
	// Stop the server
	svr->stop();
	delete svr;
    
    return 0;
}
