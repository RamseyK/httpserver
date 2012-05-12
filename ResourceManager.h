#ifndef _RESOURCEMANAGER_H_
#define _RESOURCEMANAGER_H_

#include <dirent.h>
#include <fstream>

#include "Resource.h"

class ResourceManager {
private:
    // Flag to determine if the ResourceManager only fetches from within memory
    bool memoryOnly;
    
    // Local file system base path
    std::string diskBasePath;

	// Map to track resources only in memory (not on disk)
    std::map<std::string, Resource*> *memoryFileMap;
    
private:

    // Check a URI to ensure it's within the scope of the accessible file system (or generally safe)
    bool isUriSafe(std::string uri);
    
public:
    ResourceManager(std::string base, bool memoryFS);
    ~ResourceManager();

	// Reset's the memory file system by deleteing all resources from memory and clear()ing the map
	void resetMemoryFS();
    
    // Refreshes the in memory file system by syncing it with the base path on the disk
    void refreshMemoryFS();

	// Debug, load test objects into the memory FS
	void loadTestMemory();
    
    // Provide a string rep of the directory listing
    string listDirectory(std::string dirPath);
    
    // Load a file from disk or memory and return it's representation as a Resource object
    Resource* getResource(std::string uri);

	// Any resource retrieved from getResource is expected to be "returned" for cleanup
	void returnResource(Resource *res);

	// Getters
	bool isMemoryFS() {
		return memoryOnly;
	}
};

#endif
