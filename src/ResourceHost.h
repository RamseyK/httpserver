/**
    httpserver
    ResourceHost.h
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

#ifndef _RESOURCEHOST_H_
#define _RESOURCEHOST_H_

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

#include "Resource.h"

class ResourceHost {
private:
    // Local file system base path
    std::string baseDiskPath;

private:
    // Returns a MIME type string given an extension
    std::string lookupMimeType(std::string const& ext) const;

    // Read a file from the FS and into a Resource object
    std::unique_ptr<Resource> readFile(std::string const& path, struct stat const& sb);

    // Reads a directory list or index from FS into a Resource object
    std::unique_ptr<Resource> readDirectory(std::string path, struct stat const& sb);

    // Provide a string rep of the directory listing based on URI
    std::string generateDirList(std::string const& dirPath) const;

public:
    explicit ResourceHost(std::string const& base);
    ~ResourceHost() = default;

    // Returns a Resource based on URI
    std::unique_ptr<Resource> getResource(std::string const& uri);
};

#endif
