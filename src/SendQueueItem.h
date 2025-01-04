/**
    httpserver
    SendQueueItem.h
    Copyright 2011-2025 Ramsey Kant

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

using byte = unsigned char;

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
    SendQueueItem(byte* data, unsigned int size, bool dc) : sendData(data), sendSize(size), sendOffset(0), disconnect(dc) {
    }

    ~SendQueueItem() {
        if (sendData != nullptr) {
            delete [] sendData;
            sendData = nullptr;
        }
    }

    void setOffset(unsigned int off) {
        sendOffset = off;
    }

    byte* getData() const {
        return sendData;
    }

    unsigned int getSize() const {
        return sendSize;
    }

    bool getDisconnect() const {
        return disconnect;
    }

    unsigned int getOffset() const {
        return sendOffset;
    }

};

#endif
