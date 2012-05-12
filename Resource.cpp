#include "Resource.h"

Resource::Resource(string rel, string disk, bool memoryResident) {
	relLoc = rel;
	diskLoc = disk;
	inMemory = memoryResident;
	data = NULL;
}

Resource::~Resource() {
	if(data != NULL)
		delete data;
}
