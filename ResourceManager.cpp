#include "ResourceManager.h"

ResourceManager::ResourceManager(std::string base, bool memoryFS) {
	memoryFileMap = NULL;
    memoryOnly = memoryFS;
    diskBasePath = base;
    
    // Check to see if the disk base path is a valid path
    
    
    // If a memoryFS, initialize the map and sync the server's memoryFS with the disk once
    if(memoryOnly) {
		memoryFileMap = new std::map<std::string, Resource*>();
        refreshMemoryFS();
    }
}

ResourceManager::~ResourceManager() {
	if(memoryOnly) {
		resetMemoryFS();
		// Delete the map instance
		delete memoryFileMap;
	}
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
 * Dump and delete all resources in the memory file system, then clear it out
 */
void ResourceManager::resetMemoryFS() {
	if(!memoryOnly)
		return;
    
	// Cleanup all Resource objects
	std::map<std::string, Resource*>::const_iterator it;
	for(it = memoryFileMap->begin(); it != memoryFileMap->end(); ++it) {
		delete it->second;
	}
	memoryFileMap->clear();
}

/**
 * For memory mode only:
 * Syncronize the memoryFileMap with the diskBasePath by loading all resoruces in the diskBasePath into memory
 *
 */
void ResourceManager::refreshMemoryFS() {
	if(!memoryOnly)
		return;
    
	// Reset the FS to start fresh
	resetMemoryFS();
    
    // Debug, load test resources
    loadTestMemory();
    
	// Load all files from the disk FS:
}

// Debug, load test objects into the memory FS
void ResourceManager::loadTestMemory() {
	Resource *res1 = new Resource("/hey/test2.mres", "", true);
    Resource *res2 = new Resource("/hey/dir/blank.mres", "", true);
    
	memoryFileMap->insert(std::pair<std::string, Resource*>(res1->getRelativeLocation(), res1));
    memoryFileMap->insert(std::pair<std::string, Resource*>(res2->getRelativeLocation(), res2));
}

/**
 * Reutrn a string representation of the directory provided by the relative path dirPath
 *
 * @param dirPath Relative directory path
 * @return String representation of the directory. Blank string if invalid directory
 */
std::string ResourceManager::listDirectory(std::string dirPath) {
    std::string ret = "";
    std::string tempPath;
    Resource *tempRec;
    size_t found;
    
    // Check URI
    if(!isUriSafe(dirPath))
        return "";
    
    // Memory FS:
    // Build a list of all resources that have a URI that begins with dirPath
    if(memoryOnly) {
        std::map<std::string, Resource*>::const_iterator it;
        for(it = memoryFileMap->begin(); it != memoryFileMap->end(); ++it) {
            tempPath = it->first;
            found = tempPath.find(dirPath);
            // Resource's URI falls within the directory search path (dirPath)
            if(found != std::string::npos) {
                // TODO: get properties of the in memory resource and provide them as part of the listing
                tempRec = it->second;
                
                // Add the relative path to the return with a new line
                ret += tempPath + "\n";
            }
        }
    } else { // Look in the disk FS
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
    }
    
    return ret;
}

/**
 * Retrieve a resource from the File System in use (Disk or Memory)
 * Any resource retrieved from this function should be returned after use with returnResource
 *
 * @param uri The URI sent in the request
 * @return NULL if unable to load the resource. Resource object
 */
Resource* ResourceManager::getResource(std::string uri) {
	Resource* res = NULL;
    
    // If the requested resource URI is unsafe, bail
    if(!isUriSafe(uri))
        return NULL;
    
	// If using memory file system:
	if(memoryOnly) {
		std::map<std::string, Resource*>::const_iterator it;
		it = memoryFileMap->find(uri);
		// If it isn't the element past the end (end()), then a resource was found
		if(it != memoryFileMap->end()) {
			res = it->second;
		}
	} else { // Otherwise check the disk
        std::ifstream file;
        long long len = 0;
        
        // Open file for reading as binary
        uri = diskBasePath + uri;
        file.open(uri.c_str(), std::ios::binary);
        
        // Return null if failed
        if(!file.is_open())
            return NULL;
        
        // Get the length of the file
        file.seekg(0, std::ios::end);
        len = file.tellg();
        file.seekg(0, std::ios::beg);
        
        // Allocate memory for contents of file and read in the contents
        char *fdata = new char[len];
        file.read(fdata, len);
        
        // Close the file
        file.close();
            
        // Create a new Resource object and setup it's contents
        res = new Resource(uri, uri, false);
        res->setData(fdata);
	}
    
	return res;
}

/**
 * Any resource retrieved from getResource must be sent back to this function for cleanup
 * Memory resident objects will also be passed to this function and most likely not undergo any changes however, this behavior is desired
 * in the event locks need to be released or future cleanup tasks on all requested resources need to be performed.
 *
 * @param res A resource object allocated within getResource
 *
 */
void ResourceManager::returnResource(Resource *res) {
	if(res == NULL)
		return;
    
	// If the resource is memory resident, the object doesn't need to be deleted
    // Object's loaded from the FS should released
	if(!res->isMemoryResident()) {
		delete res;
	}
}


