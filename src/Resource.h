/**
	httpserver
	Resource.h
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

#ifndef _RESOURCE_H_
#define _RESOURCE_H_

#include <string>

typedef unsigned char byte;

class Resource {

private:
    byte* data; // File data
	unsigned int size;
	std::string mimeType;
    std::string location; // Disk path location within the server
	bool directory;

public:
    Resource(std::string loc, bool dir = false);
    ~Resource();
    
    // Setters
    
    void setData(byte* d, unsigned int s) {
        data = d;
		size = s;
    }

	void setMimeType(std::string mt) {
		mimeType = mt;
	}
    
    // Getters

	std::string getMimeType() {
		return mimeType;
	}
    
    std::string getLocation() {
        return location;
    }

	bool isDirectory() {
		return directory;
	}

	byte* getData() {
		return data;
	}
	
	unsigned int getSize() {
		return size;
	}
	
	// Get the file extension
	std::string getExtension() {
		std::string ext = "";
		size_t ext_pos = location.find_last_of(".");
		if(ext_pos != std::string::npos)
			ext = location.substr(ext_pos+1);
		return ext;
	}
};

#endif
