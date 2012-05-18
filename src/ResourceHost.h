/**
	httpserver
	ResourceHost.h
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

#ifndef _RESOURCEHOST_H_
#define _RESOURCEHOST_H_

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>

#include "Resource.h"

// Valid files to serve as an index of a directory
const static char* const validIndexes[] = {
	"index.html", // 0
	"index.htm", // 1
	//"index.php", // 2
};

class ResourceHost {
private:
    // Local file system base path
    std::string baseDiskPath;

	// Map to track resources only in the memory cache (not on disk)
    std::unordered_map<std::string, Resource*> *cacheMap;
    
private:

	// Reset's the cache by deleteing all resources from memory and clearing the map
	void clearCache();

	// Loads a file from the FS into the cache as a Resource
	Resource* loadFile(std::string path, struct stat sb);
	
	// Loads a directory list or index from FS into the cache
	Resource* loadDirectory(std::string path, struct stat sb);
    
public:
    ResourceHost(std::string base);
    ~ResourceHost();
    
    // Provide a string rep of the directory listing
    std::string listDirectory(std::string dirPath);

	// Write a resource to the cache and file system
	void putResource(Resource* res, bool writeToDisk);
    
    // Returns a Resource in the cache or file system if it exists
    Resource* getResource(std::string uri);
};

#endif
