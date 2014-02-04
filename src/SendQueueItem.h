/**
	httpserver
	SendQueueItem.h
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

#ifndef _SENDQUEUEITEM_H_
#define _SENDQUEUEITEM_H_

#include <cstdlib>

typedef unsigned char byte;

/**
 * SendQueueItem
 * Object represents a piece of data in a clients send queue
 * Contains a pointer to the send buffer and tracks the current amount of data sent (by offset)
 */
class SendQueueItem {

private:
	byte* sendData;
	unsigned int sendSize;
	unsigned int sendOffset;
	bool disconnect; // Flag indicating if the client should be disconnected after this item is dequeued

public:
	SendQueueItem(byte* data, unsigned int size, bool dc) {
		sendData = data;
		sendSize = size;
		disconnect = dc;
		sendOffset = 0;
	}

	~SendQueueItem() {
		if(sendData != NULL) {
			delete [] sendData;
			sendData = NULL;
		}
	}

	void setOffset(unsigned int off) {
		sendOffset = off;
	}

	byte* getData() {
		return sendData;
	}

	unsigned int getSize() {
		return sendSize;
	}

	bool getDisconnect() {
		return disconnect;
	}

	unsigned int getOffset() {
		return sendOffset;
	}

};

#endif
