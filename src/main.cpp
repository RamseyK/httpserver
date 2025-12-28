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

#include <print>
#include <string>
#include <unordered_map>
#include <fstream>
#include <signal.h>
#include <sys/stat.h>

#include "HTTPServer.h"
#include "ResourceHost.h"

static std::unique_ptr<HTTPServer> svr;

// Ignore signals with this function
void handleSigPipe([[maybe_unused]] int32_t snum) {
    return;
}

// Termination signal handler (Ctrl-C)
void handleTermSig([[maybe_unused]] int32_t snum) {
    svr->canRun = false;
}

int main()
{
    // Parse config file
    std::map<std::string, std::string, std::less<>> config;
    std::fstream cfile;
    std::string line;
    std::string key;
    std::string val;
    int32_t epos = 0;
    cfile.open("server.config");
    if (!cfile.is_open()) {
        std::print("Unable to open server.config file in working directory\n");
        return -1;
    }
    while (getline(cfile, line)) {
        // Skip empty lines or those beginning with a #
        if (line.length() == 0 || line.rfind("#", 0) == 0)
            continue;

        epos = line.find("=");
        key = line.substr(0, epos);
        val = line.substr(epos + 1, line.length());
        config.try_emplace(key, val);
    }
    cfile.close();

    // Validate at least vhost, port, and diskpath are present
    if (!config.contains("vhost") || !config.contains("port") || !config.contains("diskpath")) {
        std::print("vhost, port, and diskpath must be supplied in the config, at a minimum\n");
        return -1;
    }

    struct stat sb = {0};
    if (stat(config["diskpath"].c_str(), &sb) != 0) {
        std::print("diskpath must exist: {}\n", config["diskpath"]);
        return -1;
    }

    // Break vhost into a comma separated list (if there are multiple vhost aliases)
    std::vector<std::string> vhosts;
    std::string vhost_alias_str = config["vhost"];
    std::string delimiter = ",";
    std::string token;
    size_t pos = 0;
    do {
        pos = vhost_alias_str.find(delimiter);
        token = vhost_alias_str.substr(0, pos);
        vhosts.push_back(token);
        vhost_alias_str.erase(0, pos + delimiter.length());
    } while (pos != std::string::npos);

    // Check for optional drop_uid, drop_gid.  Ensure both are set
    int32_t drop_uid = 0;
    int32_t drop_gid = 0;
    if (config.contains("drop_uid") && config.contains("drop_gid")) {
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
    svr = std::make_unique<HTTPServer>(vhosts, atoi(config["port"].c_str()), config["diskpath"], drop_uid, drop_gid);
    if (!svr->start()) {
        svr->stop();
        return -1;
    }

    // Run main event loop
    svr->process();

    // Stop the server
    svr->stop();

    return 0;
}
