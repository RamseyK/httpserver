#include "ResourceManager.h"

ResourceManager::ResourceManager(std::string base) {
	cacheMap = NULL;
    diskBasePath = base;
    
    // Check to see if the disk base path is a valid path
    
    
    // Initialize cache map..possibly preload cache with files as a future feature?
	cacheMap = new std::map<std::string, Resource*>();
}

ResourceManager::~ResourceManager() {
	clearCache();
	
	// Delete the map instance
	delete cacheMap;
}

/**
 * Check the URI (to a file or folder) to see if it's:
 * (1) In the file system's scope
 * (2) Has only valid characters
 * (3) Is over 255 characters
 * 
 * @return True if valid. False otherwise
 */
bool ResourceManager::isUriSafe(std::string uri) {
    // Must not be empty
    if(uri.empty())
        return false;
    
    // Too long
    if(uri.size() > 255)
        return false;
    
    return true;
}

/**
 * Load File
 * Read a file from disk and load it into the memory cache
 *
 * @param uri Request URI (relative path) of the file
 * @return Return's the resource object upon successful load
 */
Resource* ResourceManager::loadFile(std::string uri) {
	std::ifstream file;
	unsigned int len = 0;
	
	// Open the file
	std::string path = diskBasePath + uri;
	file.open(path.c_str(), std::ios::binary);
  
	// Return null if failed
	if(!file.is_open())
	    return NULL;
  
	// Get the length of the file
	file.seekg(0, std::ios::end);
	len = file.tellg();
	file.seekg(0, std::ios::beg);
  
	// Allocate memory for contents of file and read in the contents
	byte* fdata = new byte[len];
	file.read((char*)fdata, len);
  
	// Close the file
	file.close();
      
	// Create a new Resource object and setup it's contents
	Resource* res = new Resource(uri);
	res->setData(fdata, len);
	
	// Insert the resource into the map
	cacheMap->insert(std::pair<std::string, Resource*>(res->getLocation(), res));
	
	return res;
}

/**
 * Dump and delete all resources in the cache, then clear it out
 */
void ResourceManager::clearCache() {
	// Cleanup all Resource objects
	std::map<std::string, Resource*>::const_iterator it;
	for(it = cacheMap->begin(); it != cacheMap->end(); ++it) {
		delete it->second;
	}
	cacheMap->clear();
}

/**
 * Reutrn a string representation of the directory provided by the relative path dirPath
 *
 * @param dirPath Relative directory path
 * @return String representation of the directory. Blank string if invalid directory
 */
std::string ResourceManager::listDirectory(std::string dirPath) {
    // Check URI
    if(!isUriSafe(dirPath))
        return "";

	std::string ret = "";
    DIR *dir;
    struct dirent *ent;
    std::string path = diskBasePath + dirPath;
    dir = opendir(path.c_str());
    if(dir == NULL)
        return "";
    
    // Add all files and directories to the return
    while((ent = readdir(dir)) != NULL) {
        ret += ent->d_name;
        ret += "\n";
    }
    
    // Close the directory
    closedir(dir);
    
    return ret;
}

/**
 * Retrieve a resource from the File system
 * The memory cache will be checked before going out to disk
 *
 * @param uri The URI sent in the request
 * @return NULL if unable to load the resource. Resource object
 */
Resource* ResourceManager::getResource(std::string uri) {
	Resource* res = NULL;
    
    // If the requested resource URI is unsafe, bail
    if(!isUriSafe(uri))
        return NULL;
    
	// Check the cache first:
	std::map<std::string, Resource*>::const_iterator it;
	it = cacheMap->find(uri);
	// If it isn't the element past the end (end()), then a resource was found
	if(it != cacheMap->end()) {
		res = it->second;
		return res;
	}

	// Not in cache, check the disk
	// Attempt to load the resource into memory from the FS (will be NULL if not found)
	return loadFile(uri);
}
