#ifndef _RESOURCE_H
#define _RESOURCE_H

#include <string>

typedef unsigned char byte;

class Resource {

private:
    byte* data; // File data
	unsigned int size;
    std::string encoding;
    std::string language;
    std::string location; // Resource location within the server
    std::string md5;

public:
    Resource(std::string loc);
    ~Resource();
    
    // Setters
    
    void setData(byte* d, unsigned int s) {
        data = d;
		size = s;
    }
    
    // Getters
    
    std::string getEncoding() {
        return encoding;
    }
    
    std::string getLanguage() {
        return language;
    }
    
    std::string getLocation() {
        return location;
    }

	byte* getData() {
		return data;
	}
	
	unsigned int getSize() {
		return size;
	}
};

#endif
