/**
	httpserver
	ResourceHost.cpp
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

#include "ResourceHost.h"

ResourceHost::ResourceHost(std::string base) {
	cacheMap = NULL;
    diskBasePath = base;
    
    // Check to see if the disk base path is a valid path
    
    
    // Initialize cache map..possibly preload cache with files as a future feature?
	cacheMap = new std::map<std::string, Resource*>();
}

ResourceHost::~ResourceHost() {
	clearCache();
	
	// Delete the map instance
	delete cacheMap;
}

/**
 * Load File
 * Read a file from disk and load it into the memory cache
 *
 * @param path Full disk path of the file
 * @param sb Filled in stat struct
 * @return Return's the resource object upon successful load
 */
Resource* ResourceHost::loadFile(std::string path, struct stat sb) {
	std::ifstream file;
	unsigned int len = 0;
	
	// Open the file
	file.open(path.c_str(), std::ios::binary);
  
	// Return null if failed
	if(!file.is_open())
	    return NULL;
  
	// Get the length of the file
	/*file.seekg(0, std::ios::end);
	len = file.tellg();
	file.seekg(0, std::ios::beg);*/
	len = sb.st_size;
  
	// Allocate memory for contents of file and read in the contents
	byte* fdata = new byte[len];
	file.read((char*)fdata, len);
  
	// Close the file
	file.close();
      
	// Create a new Resource object and setup it's contents
	Resource* res = new Resource(path);
	res->setData(fdata, len);
	
	// Insert the resource into the map
	cacheMap->insert(std::pair<std::string, Resource*>(res->getLocation(), res));
	
	return res;
}

/**
 * Dump and delete all resources in the cache, then clear it out
 */
void ResourceHost::clearCache() {
	// Cleanup all Resource objects
	std::map<std::string, Resource*>::const_iterator it;
	for(it = cacheMap->begin(); it != cacheMap->end(); ++it) {
		delete it->second;
	}
	cacheMap->clear();
}

/**
 * Return an HTML directory listing provided by the relative path dirPath
 *
 * @param path Full disk path of the file
 * @param uri Relative webserver URI
 * @return String representation of the directory. Blank string if invalid directory
 */
std::string ResourceHost::listDirectory(std::string path, std::string uri) {
	std::stringstream ret;
	ret << "<html><head><title>" << uri << "</title></head><body>";
	
    DIR *dir;
    struct dirent *ent;

    dir = opendir(path.c_str());
    if(dir == NULL)
        return "";

	// Page title, displaying the URI of the directory being listed
	ret << "<h1>Index of " << uri << "</h1><hr><br />";
    
    // Add all files and directories to the return
    while((ent = readdir(dir)) != NULL) {
		// Skip any 'hidden' files (starting with a '.')
		if(ent->d_name[0] == '.')
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
 * The memory cache will be checked before going out to disk
 *
 * @param uri The URI sent in the request
 * @return NULL if unable to load the resource. Resource object
 */
Resource* ResourceHost::getResource(std::string uri) {
	if(uri.length() > 255 || uri.empty())
		return NULL;
	
	std::string path = diskBasePath + uri;
	Resource* res = NULL;
    
	// Check the cache first:
	std::map<std::string, Resource*>::const_iterator it;
	it = cacheMap->find(path);
	// If it isn't the element past the end (end()), then a resource was found
	if(it != cacheMap->end()) {
		res = it->second;
		return res;
	}

	// Not in cache, check the disk
	
	// Gather info about the resource with stat: determine if it's a directory or file, check if its owned by group/user, modify times
	struct stat sb;
	if(stat(path.c_str(), &sb) == -1)
		return NULL; // File not found
	
	// Make sure the webserver USER owns the files
	if(!(sb.st_mode & S_IRWXU))
		return NULL;
	
	// Determine file type
	if(sb.st_mode & S_IFDIR) { // Directory
		// Generate an HTML directory listing
		std::string listing = listDirectory(path, uri);
		unsigned int slen = listing.length();
		char* sdata = new char[slen];
		strncpy(sdata, listing.c_str(), slen);
		
		res = new Resource(path, true);
		res->setData((byte*)sdata, slen);
		
		// Cache the listing
		cacheMap->insert(std::pair<std::string, Resource*>(res->getLocation(), res));
	} else if(sb.st_mode & S_IFREG) { // Regular file
		// Attempt to load the file into memory from the FS
		res = loadFile(path, sb);
	} else { // Something else..device, socket, symlink
		return NULL;
	}

	return res;
}
