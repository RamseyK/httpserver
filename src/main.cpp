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

#include <charconv>
#include <map>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <unordered_map>
#include <fstream>
#include <csignal>
#include <sys/stat.h>

#include "HTTPServer.h"
#include "ResourceHost.h"

static std::unique_ptr<HTTPServer> svr;

void handleSigPipe([[maybe_unused]] int snum) {
    // Intentionally empty — suppress SIGPIPE without side effects
}

void handleTermSig([[maybe_unused]] int snum) {
    // canRun is volatile bool — the write is visible to the process() loop.
    // No std::print or non-trivial calls here; only the flag write.
    if (svr) svr->canRun = false;
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

    if (struct stat sb = {0}; stat(config["diskpath"].c_str(), &sb) != 0) {
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

    // Helper: parse a decimal integer from a string, returns nullopt on any error
    auto parse_int = [](std::string_view s) -> std::optional<int32_t> {
        int32_t val = 0;
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
        if (ec != std::errc{} || ptr != s.data() + s.size())
            return std::nullopt;
        return val;
    };

    // Check for optional drop_uid, drop_gid.  Ensure both are set
    int32_t drop_uid = 0;
    int32_t drop_gid = 0;
    if (config.contains("drop_uid") && config.contains("drop_gid")) {
        auto uid_opt = parse_int(config["drop_uid"]);
        auto gid_opt = parse_int(config["drop_gid"]);

        if (!uid_opt || !gid_opt || *uid_opt <= 0 || *gid_opt <= 0) {
            std::print("drop_uid and drop_gid must both be positive integers\n");
            return -1;
        }

        drop_uid = *uid_opt;
        drop_gid = *gid_opt;
    }

    // Ignore SIGPIPE "Broken pipe" signals when socket connections are broken.
    signal(SIGPIPE, handleSigPipe);

    // Register termination signals
    signal(SIGABRT, &handleTermSig);
    signal(SIGINT, &handleTermSig);
    signal(SIGTERM, &handleTermSig);

    auto port_opt = parse_int(config["port"]);
    if (!port_opt || *port_opt <= 0 || *port_opt > 65535) {
        std::print("port must be a valid integer between 1 and 65535\n");
        return -1;
    }

    // Instantiate and start the server
    svr = std::make_unique<HTTPServer>(vhosts, *port_opt, config["diskpath"], drop_uid, drop_gid);
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
