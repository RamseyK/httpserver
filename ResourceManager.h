/**
	httpserver
	ResourceManager.h
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

#ifndef _RESOURCEMANAGER_H_
#define _RESOURCEMANAGER_H_

#include <string>
#include <map>
#include <dirent.h>
#include <fstream>

#include "Resource.h"

class ResourceManager {
private:
    // Local file system base path
    std::string diskBasePath;

	// Map to track resources only in the memory cache (not on disk)
    std::map<std::string, Resource*> *cacheMap;
    
private:

    // Check a URI to ensure it's within the scope of the accessible file system (or generally safe)
    bool isUriSafe(std::string uri);

	// Reset's the cache by deleteing all resources from memory and clearing the map
	void clearCache();

	// Loads a file from the FS into the cache as a Resource
	Resource* loadFile(std::string path);
    
public:
    ResourceManager(std::string base);
    ~ResourceManager();
    
    // Provide a string rep of the directory listing
    std::string listDirectory(std::string dirPath);

	// Write a resource to the cache and file system
	void putResource(Resource* res, bool writeToDisk);
    
    // Returns a Resource in the cache or file system if it exists
    Resource* getResource(std::string uri);
};

#endif
