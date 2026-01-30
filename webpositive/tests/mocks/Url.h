#ifndef _MOCK_URL_H
#define _MOCK_URL_H

#include "String.h"

class BUrl {
public:
    BUrl(const char* url) : fUrl(url) {}
    bool IsValid() const { return fUrl.Length() > 0; }
    void SetPath(const char* path) { fPath = path; }
    BString Path() const { return fPath; }
private:
    BString fUrl;
    BString fPath;
};

#endif
