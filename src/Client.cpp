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

Client::Client(SOCKET fd, sockaddr_in addr) {
    socketDesc = fd;
    clientAddr = addr;
}

Client::~Client() {
    clearSendQueue();
}

void Client::addToSendQueue(SendQueueItem* item) {
	sendQueue.push(item);
}

unsigned int Client::sendQueueSize() {
	return sendQueue.size();
}

SendQueueItem* Client::nextFromSendQueue(unsigned int bytesToSend) {
	if(sendQueue.empty())
		return NULL;

	SendQueueItem* item = sendQueue.front();
	unsigned int remaining = item->getSize() - item->getOffset();


	if(bytesToSend > remaining) { // Rest or entire resource data can be sent, Dequeue
		sendQueue.pop(); // Dequeue
	} else { // Only a portion of the resource can be sent, keep in queue
		// Add to the back of the queue
		sendQueue.pop();
		sendQueue.push(item);
	}

	return item;
}
    
void Client::clearSendQueue() {
	while(!sendQueue.empty()) {
		delete sendQueue.front();
		sendQueue.pop();
	}
}