/**
	httpserver
	Client.cpp
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

#include "Client.h"

Client::Client(int fd, sockaddr_in addr) {
    socketDesc = fd;
    clientAddr = addr;
}

Client::~Client() {
    clearSendQueue();
}

/**
 * Add to Send Queue
 * Adds a SendQueueItem object to the end of this client's send queue
 */
void Client::addToSendQueue(SendQueueItem* item) {
	sendQueue.push(item);
}

/**
 * sendQueueSize
 * Returns the current number of SendQueueItem's in the send queue
 *
 * @return Integer representing number of items in this clients send queue
 */
unsigned int Client::sendQueueSize() {
	return sendQueue.size();
}

/**
 * Next from Send Queue
 * Returns the current SendQueueItem object to be sent to the client
 *
 * @return SendQueueItem object containing the data to send and current offset
 */
SendQueueItem* Client::nextInSendQueue() {
	if(sendQueue.empty())
		return NULL;

	return sendQueue.front();
}

/**
 * Dequeue from Send Queue
 * Deletes and dequeues first item in the queue
 */
void Client::dequeueFromSendQueue() {
	SendQueueItem* item = nextInSendQueue();
	if(item != NULL) {
		sendQueue.pop();
		delete item;
	}
}

/**
 * Clear Send Queue
 * Clears out the send queue for the client, deleting all internal SendQueueItem objects
 */
void Client::clearSendQueue() {
	while(!sendQueue.empty()) {
		delete sendQueue.front();
		sendQueue.pop();
	}
}
