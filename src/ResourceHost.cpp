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

#include <memory>
#include <sstream>
#include <string>
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

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

    std::ifstream file;
    uint32_t len = 0;

    // Open the file
    file.open(path, std::ios::binary);

    // Return null if the file failed to open
    if (!file.is_open())
        return nullptr;

    // Get the length of the file
    len = sb.st_size;

    // Allocate memory for contents of file and read in the contents
    auto fdata = new uint8_t[len];
    memset(fdata, 0x00, len);
    file.read((char*)fdata, len);

    // Close the file
    file.close();

    if (auto mimetype = lookupMimeType(resource->getExtension()); mimetype.length() != 0) {
        resource->setMimeType(mimetype);
    } else {
        resource->setMimeType("application/octet-stream");  // default to binary
    }

    resource->setData(fdata, len);

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
    auto sdata = new uint8_t[slen];
    memset(sdata, 0x00, slen);
    strncpy((char*)sdata, listing.c_str(), slen);

    auto resource = std::make_unique<Resource>(path, true);
    resource->setMimeType("text/html");
    resource->setData(sdata, slen);

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

    std::stringstream ret;
    ret << "<html><head><title>" << uri << "</title></head><body>";

    const struct dirent* ent = nullptr;
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr)
        return "";

    // Page title, displaying the URI of the directory being listed
    ret << "<h1>Index of " << uri << "</h1><hr /><br />";

    // Add all files and directories to the return
    while ((ent = readdir(dir)) != nullptr) {
        // Skip any 'hidden' files (starting with a '.')
        if (ent->d_name[0] == '.')
            continue;

        // Display link to object in directory:
        ret << "<a href=\"" << uri << ent->d_name << "\">" << ent->d_name << "</a><br />";
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
std::unique_ptr<Resource> ResourceHost::getResource(std::string const& uri) {
    if (uri.length() > 255 || uri.empty())
        return nullptr;

    // Do not allow directory traversal
    if (uri.contains("../") || uri.contains("/.."))
        return nullptr;

    // Gather info about the resource with stat: determine if it's a directory or file, check if its owned by group/user, modify times
    std::string path = baseDiskPath + uri;
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
