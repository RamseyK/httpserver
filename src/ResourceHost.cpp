/**
    httpserver
    ResourceHost.cpp
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

#include "ResourceHost.h"

#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// Escape special HTML characters to prevent XSS in generated directory listings
static std::string htmlEscape(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&#39;";  break;
            default:   out += c;        break;
        }
    }
    return out;
}

// Valid files to serve as an index of a directory
const static std::vector<std::string> g_validIndexes = {
    "index.html",
    "index.htm"
};

// Dictionary that relates file extensions to their MIME type
const static std::unordered_map<std::string, std::string, std::hash<std::string>, std::equal_to<>> g_mimeMap = {
#include "MimeTypes.inc"
};

ResourceHost::ResourceHost(std::string const& base) : baseDiskPath(base) {
}

/**
 * Looks up a MIME type in the dictionary
 *
 * @param ext File extension to use for the lookup
 * @return MIME type as a String. If type could not be found, returns an empty string
 */
std::string ResourceHost::lookupMimeType(std::string const& ext) const {
    auto it = g_mimeMap.find(ext);
    if (it == g_mimeMap.end())
        return "";

    return it->second;
}

/**
 * Read File
 * Read a file from disk and return the appropriate Resource object
 * This creates a new Resource object - callers are expected to dispose of the return value if non-NULL
 *
 * @param path Full disk path of the file
 * @param sb Filled in stat struct
 * @return Return's the resource object upon successful load
 */
std::unique_ptr<Resource> ResourceHost::readFile(std::string const& path, struct stat const& sb) {
    // Make sure the webserver USER owns the file
    if (!(sb.st_mode & S_IRWXU))
        return nullptr;

    // Create a new Resource object and setup it's contents
    auto resource = std::make_unique<Resource>(path);
    std::string name = resource->getName();
    if (name.length() == 0) {
        return nullptr;  // Malformed name
    }

    // Always disallow hidden files
    if (name.starts_with(".")) {
        return nullptr;
    }

    // Reject files that would overflow uint32_t or are unreasonably large (256 MB limit)
    constexpr off_t MAX_FILE_SIZE = 256 * 1024 * 1024;
    if (sb.st_size < 0 || sb.st_size > MAX_FILE_SIZE)
        return nullptr;
    uint32_t len = static_cast<uint32_t>(sb.st_size);

    std::ifstream file;

    // Open the file
    file.open(path, std::ios::binary);

    // Return null if the file failed to open
    if (!file.is_open())
        return nullptr;

    // Allocate memory for contents of file and read in the contents
    auto fdata = std::make_unique<uint8_t[]>(len);
    file.read(reinterpret_cast<char*>(fdata.get()), len);

    // Close the file
    file.close();

    if (auto mimetype = lookupMimeType(resource->getExtension()); mimetype.length() != 0) {
        resource->setMimeType(mimetype);
    } else {
        resource->setMimeType("application/octet-stream");  // default to binary
    }

    resource->setData(std::move(fdata), len);

    return resource;
}

/**
 * Read Directory
 * Read a directory (list or index) from disk into a Resource object
 * This creates a new Resource object - callers are expected to dispose of the return value if non-NULL
 *
 * @param path Full disk path of the file
 * @param sb Filled in stat struct
 * @return Return's the resource object upon successful load
 */
std::unique_ptr<Resource> ResourceHost::readDirectory(std::string path, struct stat const& sb) {
    // Make the path end with a / (for consistency) if it doesnt already
    if (path.empty() || path[path.length() - 1] != '/')
        path += "/";

    // Probe for valid indexes
    uint32_t numIndexes = std::size(g_validIndexes);
    std::string loadIndex;
    struct stat sidx = {0};
    for (uint32_t i = 0; i < numIndexes; i++) {
        loadIndex = path + g_validIndexes[i];
        // Found a suitable index file to load and return to the client
        if (stat(loadIndex.c_str(), &sidx) == 0)
            return readFile(loadIndex, sidx);
    }

    // Make sure the webserver USER owns the directory
    if (!(sb.st_mode & S_IRWXU))
        return nullptr;

    // Generate an HTML directory listing
    std::string listing = generateDirList(path);

    uint32_t slen = listing.length();
    auto sdata = std::make_unique<uint8_t[]>(slen);
    std::memcpy(sdata.get(), listing.data(), slen);

    auto resource = std::make_unique<Resource>(path, true);
    resource->setMimeType("text/html");
    resource->setData(std::move(sdata), slen);

    return resource;
}

/**
 * Return an HTML directory listing provided by the relative path dirPath
 *
 * @param path Full disk path of the file
 * @return HTML string representation of the directory. Blank string if invalid directory
 */
std::string ResourceHost::generateDirList(std::string const& path) const {
    // Get just the relative uri from the entire path by stripping out the baseDiskPath from the beginning
    size_t uri_pos = path.find(baseDiskPath);
    std::string uri = "?";
    if (uri_pos != std::string::npos)
        uri = path.substr(uri_pos + baseDiskPath.length());

    std::string escaped_uri = htmlEscape(uri);

    std::stringstream ret;
    ret << "<html><head><title>" << escaped_uri << "</title></head><body>";

    const struct dirent* ent = nullptr;
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr)
        return "";

    // Page title, displaying the URI of the directory being listed
    ret << "<h1>Index of " << escaped_uri << "</h1><hr /><br />";

    // Add all files and directories to the return
    while ((ent = readdir(dir)) != nullptr) {
        // Skip any 'hidden' files (starting with a '.')
        if (ent->d_name[0] == '.')
            continue;

        // Display link to object in directory:
        std::string escaped_name = htmlEscape(ent->d_name);
        ret << "<a href=\"" << escaped_uri << escaped_name << "\">" << escaped_name << "</a><br />";
    }

    // Close the directory
    closedir(dir);

    ret << "</body></html>";

    return ret.str();
}

/**
 * Retrieve a resource from the File system
 * This returns a new Resource object - callers are expected to dispose of the return value if non-NULL
 *
 * @param uri The URI sent in the request
 * @return NULL if unable to load the resource. Resource object
 */
std::unique_ptr<Resource> ResourceHost::getResource(std::string_view uri) {
    if (uri.length() > 255 || uri.empty())
        return nullptr;

    // Resolve canonical paths to prevent traversal via "..", symlinks, or encoded sequences.
    // std::filesystem::canonical requires the path to exist, which also acts as an existence pre-check.
    std::error_code ec;
    std::filesystem::path canonical_base = std::filesystem::canonical(baseDiskPath, ec);
    if (ec) return nullptr;

    std::filesystem::path canonical_path = std::filesystem::canonical(std::filesystem::path(baseDiskPath) / uri, ec);
    if (ec) return nullptr; // Path does not exist or symlink loop

    // Verify every component of canonical_base is a prefix of canonical_path
    auto [base_end, path_end] = std::mismatch(
        canonical_base.begin(), canonical_base.end(),
        canonical_path.begin(), canonical_path.end()
    );
    if (base_end != canonical_base.end())
        return nullptr;

    std::string path = canonical_path.native();
    struct stat sb = {0};
    if (stat(path.c_str(), &sb) != 0)
        return nullptr; // File not found

    // Determine file type
    if (sb.st_mode & S_IFDIR) { // Directory
        // Read a directory list or index into memory from FS
        return readDirectory(path, sb);
    } else if (sb.st_mode & S_IFREG) { // Regular file
        // Attempt to load the file into memory from the FS
        return readFile(path, sb);
    } else {
        // Something else..device, socket, symlink
    }

    return nullptr;
}
