#include "Resource.h"

Resource::Resource(std::string loc) {
	location = loc;
	encoding = "";
	language = "";
	md5 = "";
	size = 0;
	data = NULL;
}

Resource::~Resource() {
	if(data != NULL)
		delete data;
}
