/**
	httpserver
	ResourceHost.cpp
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

#include "ResourceHost.h"

ResourceHost::ResourceHost(std::string base) {
    baseDiskPath = base;
    
    // Check to see if the disk base path is a valid path
    
}

ResourceHost::~ResourceHost() {
}

/**
 * Looks up a MIME type in the dictionary
 * 
 * @param ext File extension to use for the lookup
 * @return MIME type as a String. If type could not be found, returns the default 'text/html'
 */
std::string ResourceHost::lookupMimeType(std::string ext) {
	std::unordered_map<std::string, std::string>::const_iterator it = mimeMap.find(ext);
	if(it == mimeMap.end())
		return "text/html";

	return it->second;
}

/**
 * Read File
 * Read a file from disk and return the appropriate Resource object
 *
 * @param path Full disk path of the file
 * @param sb Filled in stat struct
 * @return Return's the resource object upon successful load
 */
Resource* ResourceHost::readFile(std::string path, struct stat sb) {
	// Make sure the webserver USER owns the file
	if(!(sb.st_mode & S_IRWXU))
		return NULL;
	
	std::ifstream file;
	unsigned int len = 0;
	
	// Open the file
	file.open(path.c_str(), std::ios::binary);
  
	// Return null if the file failed to open
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
	res->setMimeType(lookupMimeType(res->getExtension()));
	res->setData(fdata, len);
	
	return res;
}

/**
 * Read Directory
 * Read a directory (list or index) from disk into a Resource object
 *
 * @param path Full disk path of the file
 * @param sb Filled in stat struct
 * @return Return's the resource object upon successful load
 */
Resource* ResourceHost::readDirectory(std::string path, struct stat sb) {
	Resource* res = NULL;
	// Make the path end with a / (for consistency) if it doesnt already
	if(path.empty() || path[path.length()-1] != '/')
		path += "/";
	
	// Probe for valid indexes
	int numIndexes = sizeof(validIndexes) / sizeof(*validIndexes);
	std::string loadIndex;
	struct stat sidx;
	for(int i = 0; i < numIndexes; i++) {
		loadIndex = path + validIndexes[i];
		// Found a suitable index file to load and return to the client
		if(stat(loadIndex.c_str(), &sidx) != -1)
			return readFile(loadIndex.c_str(), sidx);
	}
	
	// Make sure the webserver USER owns the directory
	if(!(sb.st_mode & S_IRWXU))
		return NULL;
	
	// Generate an HTML directory listing
	std::string listing = generateDirList(path);
	
	unsigned int slen = listing.length();
	char* sdata = new char[slen];
	strncpy(sdata, listing.c_str(), slen);
	
	res = new Resource(path, true);
	res->setData((byte*)sdata, slen);
	
	return res;
}

/**
 * Return an HTML directory listing provided by the relative path dirPath
 *
 * @param path Full disk path of the file
 * @return HTML string representation of the directory. Blank string if invalid directory
 */
std::string ResourceHost::generateDirList(std::string path) {
	// Get just the relative uri from the entire path by stripping out the baseDiskPath from the beginning
	size_t uri_pos = path.find_last_of(baseDiskPath);
	std::string uri = "?";
	if(uri_pos != std::string::npos)
		uri = path.substr(uri_pos);
	
	std::stringstream ret;
	ret << "<html><head><title>" << uri << "</title></head><body>";
	
    DIR *dir;
    struct dirent *ent;

    dir = opendir(path.c_str());
    if(dir == NULL)
        return "";

	// Page title, displaying the URI of the directory being listed
	ret << "<h1>Index of " << uri << "</h1><hr /><br />";
    
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
 *
 * @param uri The URI sent in the request
 * @return NULL if unable to load the resource. Resource object
 */
Resource* ResourceHost::getResource(std::string uri) {
	if(uri.length() > 255 || uri.empty())
		return NULL;
	
	std::string path = baseDiskPath + uri;
	Resource* res = NULL;
	
	// Gather info about the resource with stat: determine if it's a directory or file, check if its owned by group/user, modify times
	struct stat sb;
	if(stat(path.c_str(), &sb) == -1)
		return NULL; // File not found
	
	// Determine file type
	if(sb.st_mode & S_IFDIR) { // Directory
		// Read a directory list or index into memory from FS
		res = readDirectory(path, sb);
	} else if(sb.st_mode & S_IFREG) { // Regular file
		// Attempt to load the file into memory from the FS
		res = readFile(path, sb);
	} else { // Something else..device, socket, symlink
		return NULL;
	}

	return res;
}
