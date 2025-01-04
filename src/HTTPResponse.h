/**
 ByteBuffer
 HTTPResponse.h
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

#ifndef _HTTPRESPONSE_H_
#define _HTTPRESPONSE_H_

#include "HTTPMessage.h"

class HTTPResponse : public HTTPMessage {
private:
    // Response variables
    int status = 0;
    std::string reason = "";

    void determineReasonStr();
    void determineStatusCode();

public:
    HTTPResponse();
    explicit HTTPResponse(std::string const& sData);
    explicit HTTPResponse(byte *pData, unsigned int len);
    ~HTTPResponse() = default;

    byte* create() final;
    bool parse() final;

    // Accessors & Mutators

    void setStatus (int scode) {
        status = scode;
        determineReasonStr();
    }

    std::string getReason() const {
        return reason;
    }
};

#endif
