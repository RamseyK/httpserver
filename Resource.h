/**
	httpserver
	Resource.h
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
