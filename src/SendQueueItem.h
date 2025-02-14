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

#include <cstdint>
#include <memory>

/**
 * SendQueueItem
 * Object represents a piece of data in a clients send queue
 * Contains a pointer to the send buffer and tracks the current amount of data sent (by offset)
 */
class SendQueueItem {

private:
    std::unique_ptr<uint8_t[]> sendData;
    uint32_t sendSize;
    uint32_t sendOffset = 0;
    bool disconnect; // Flag indicating if the client should be disconnected after this item is dequeued

public:
    SendQueueItem(std::unique_ptr<uint8_t[]> data, uint32_t size, bool dc) : sendData(std::move(data)), sendSize(size), disconnect(dc) {
    }

    ~SendQueueItem() = default;
    SendQueueItem(SendQueueItem const&) = delete;  // Copy constructor
    SendQueueItem& operator=(SendQueueItem const&) = delete;  // Copy assignment
    SendQueueItem(SendQueueItem &&) = delete;  // Move
    SendQueueItem& operator=(SendQueueItem &&) = delete;  // Move assignment

    void setOffset(uint32_t off) {
        sendOffset = off;
    }

    uint8_t* getRawDataPointer() const {
        return sendData.get();
    }

    uint32_t getSize() const {
        return sendSize;
    }

    bool getDisconnect() const {
        return disconnect;
    }

    uint32_t getOffset() const {
        return sendOffset;
    }

};

#endif
