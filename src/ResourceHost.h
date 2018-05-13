/**
	httpserver
	ResourceHost.h
	Copyright 2011-2014 Ramsey Kant

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

#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <unordered_map>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>

#include "Resource.h"

// Valid files to serve as an index of a directory
const static char* const validIndexes[] = {
	"index.html",
	"index.htm"
};

class ResourceHost {
private:
    // Local file system base path
    std::string baseDiskPath;

	// Dictionary that relates file extensions to their MIME type
	std::unordered_map<std::string, std::string> mimeMap = {
		#define STR_PAIR(K,V) std::pair<std::string, std::string>(K,V)
		#include "MimeTypes.inc"
	};
    
private:
	// Returns a MIME type string given an extension
	std::string lookupMimeType(std::string ext);

	// Read a file from the FS and into a Resource object
	Resource* readFile(std::string path, struct stat sb);
	
	// Reads a directory list or index from FS into a Resource object
	Resource* readDirectory(std::string path, struct stat sb);
	
	// Provide a string rep of the directory listing based on URI
    std::string generateDirList(std::string dirPath);
    
public:
    ResourceHost(std::string base);
    ~ResourceHost();

	// Write a resource to the file system
	void putResource(Resource* res, bool writeToDisk);
    
    // Returns a Resource based on URI
    Resource* getResource(std::string uri);
};

#endif
