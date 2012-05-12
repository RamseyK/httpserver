#ifndef _RESOURCE_H
#define _RESOURCE_H

#include <string>

class Resource {

private:
    char *data; // Used only for resources stored completely in memory
    bool valid;
	bool inMemory; // Flag set to true if the resource is available in memory (data)
    std::string encoding;
    std::string language;
    std::string diskLoc; // Actual location on disk
    std::string relLoc; // Resource location within the server
    std::string md5;

public:
    Resource(std::string rel, std::string disk, bool memoryResident);
    ~Resource();
    
    // Setters
    
    void setData(char *d) {
        data = d;
    }
    
    // Getters

	inline bool isMemoryResident() {
		return inMemory;
	}
    
    inline std::string getEncoding() {
        return encoding;
    }
    
    inline std::string getLanguage() {
        return language;
    }
    
    inline std::string getDiskLocation() {
        return diskLoc;
    }
    
    inline std::string getRelativeLocation() {
        return relLoc;
    }
    
    // Determines if the file actually exists in the file system
    inline bool isValid() {
        return valid;
    }
};

#endif
