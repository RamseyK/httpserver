/**
 ByteBuffer
 HTTPRequest.h
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

#ifndef _HTTPREQUEST_H_
#define _HTTPREQUEST_H_

#include "HTTPMessage.h"

class HTTPRequest : public HTTPMessage {
private:
    int method = 0;
    std::string requestUri = "";

public:
    HTTPRequest();
    HTTPRequest(std::string const& sData);
    HTTPRequest(byte *pData, unsigned int len);
    virtual ~HTTPRequest();

    virtual byte *create();
    virtual bool parse();

    // Helper functions

    int methodStrToInt(std::string_view name);
    std::string methodIntToStr(unsigned int mid);

    // Info getters  & setters

    void setMethod(int m) {
        method = m;
    }

    int getMethod() const {
        return method;
    }

    void setRequestUri(std::string_view u) {
        requestUri = u;
    }

    std::string getRequestUri() const {
        return requestUri;
    }
};

#endif
