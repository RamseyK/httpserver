/**
    httpserver
    main.cpp
    Copyright 2011-2025 Ramsey Kant

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
    int epos = 0;
    int drop_uid = 0, drop_gid = 0;
    cfile.open("server.config");
    if (!cfile.is_open()) {
        std::cout << "Unable to open server.config file in working directory" << std::endl;
        return -1;
    }
    while (getline(cfile, line)) {
        // Skip empty lines or those beginning with a #
        if (line.length() == 0 || line.rfind("#", 0) == 0)
            continue;

        epos = line.find("=");
        key = line.substr(0, epos);
        val = line.substr(epos + 1, line.length());
        config.insert(std::pair<std::string, std::string> (key, val));
    }
    cfile.close();

    // Validate at least vhost, port, and diskpath are present
    auto it_vhost = config.find("vhost");
    auto it_port = config.find("port");
    auto it_path = config.find("diskpath");
    if (it_vhost == config.end() || it_port == config.end() || it_path == config.end()) {
        std::cout << "vhost, port, and diskpath must be supplied in the config, at a minimum" << std::endl;
        return -1;
    }

    // Break vhost into a comma separated list (if there are multiple vhost aliases)
    std::vector<std::string> vhosts;
    std::string vhost_alias_str = config["vhost"];
    std::string delimiter = ",";
    std::string token;
    size_t pos = vhost_alias_str.find(delimiter);
    do {
        pos = vhost_alias_str.find(delimiter);
        token = vhost_alias_str.substr(0, pos);
        vhosts.push_back(token);
        vhost_alias_str.erase(0, pos + delimiter.length());
    } while (pos != std::string::npos);

    // Check for optional drop_uid, drop_gid.  Ensure both are set
    if (config.find("drop_uid") != config.end() && config.find("drop_gid") != config.end()) {
        drop_uid = atoi(config["drop_uid"].c_str());
        drop_gid = atoi(config["drop_gid"].c_str());

        if (drop_uid <= 0 || drop_gid <= 0) {
            // Both must be set, otherwise set back to 0 so we dont use
            drop_uid = drop_gid = 0;
        }
    }

    // Ignore SIGPIPE "Broken pipe" signals when socket connections are broken.
    signal(SIGPIPE, handleSigPipe);

    // Register termination signals
    signal(SIGABRT, &handleTermSig);
    signal(SIGINT, &handleTermSig);
    signal(SIGTERM, &handleTermSig);

    // Instantiate and start the server
    svr = new HTTPServer(vhosts, atoi(config["port"].c_str()), config["diskpath"], drop_uid, drop_gid);
    if (!svr->start()) {
        svr->stop();
        delete svr;
        return -1;
    }

    // Run main event loop
    svr->process();

    // Stop the server
    svr->stop();
    delete svr;

    return 0;
}
