#include "Resource.h"

Resource::Resource(std::string rel, std::string disk, bool memoryResident) {
	relLoc = rel;
	diskLoc = disk;
	inMemory = memoryResident;
	data = NULL;
}

Resource::~Resource() {
	if(data != NULL)
		delete data;
}
