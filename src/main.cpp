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
#include <unordered_map>
#include <fstream>
#include <signal.h>

#include "HTTPServer.h"
#include "ResourceHost.h"

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
	// Parse config file
	std::map<std::string, std::string> config;
	std::fstream cfile;
	std::string line, key, val;
	int epos;
	cfile.open("server.config");
	if (!cfile.is_open()) {
		printf("Unable to open server.config file in working directory\n");
		return -1;
	}
	while (getline(cfile, line)) {
		// Skip lines beginning with a #
		if (line.rfind("#", 0) == 0)
			continue;

		epos = line.find("=");
		key = line.substr(0, epos);
		val = line.substr(epos+1, line.length());
		config.insert(std::pair<std::string, std::string> (key, val));
	}
	cfile.close();

	// Validate at least vhost, port, and diskpath are present
	auto it_vhost = config.find("vhost");
	auto it_port = config.find("port");
	auto it_path = config.find("diskpath");
	if (it_vhost == config.end() || it_port == config.end() || it_path == config.end()) {
		printf("vhost, port, and diskpath must be supplied in the config, at a minimum\n");
		return -1;
	}

    // Ignore SIGPIPE "Broken pipe" signals when socket connections are broken.
    signal(SIGPIPE, handleSigPipe);

	// Register termination signals
	signal(SIGABRT, &handleTermSig);
	signal(SIGINT, &handleTermSig);
	signal(SIGTERM, &handleTermSig);

    // Instance and start the server
	svr = new HTTPServer(config["vhost"], atoi(config["port"].c_str()), config["diskpath"]);
	svr->start();

	// Run main event loop
	svr->process();
	
	// Stop the server
	svr->stop();
	delete svr;
    
    return 0;
}
