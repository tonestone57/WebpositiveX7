#ifndef _MOCK_URL_H
#define _MOCK_URL_H

#include "String.h"

class BUrl {
public:
    BUrl(const char* url, bool parse = false) : fUrl(url) {
        (void)parse;
        // Simple host parsing for mocking purposes
        // Assumes http://host/path or https://host/path
        const char* str = fUrl.String();
        const char* protocolEnd = strstr(str, "://");
        if (protocolEnd) {
            const char* hostStart = protocolEnd + 3;
            const char* pathStart = strchr(hostStart, '/');
            if (pathStart) {
                fHost.SetTo(hostStart, pathStart - hostStart);
            } else {
                fHost = hostStart;
            }
        } else {
            fHost = str;
        }
    }
    bool IsValid() const { return fUrl.Length() > 0; }
    void SetPath(const char* path) { fPath = path; }
    BString Path() const { return fPath; }
    BString Host() const { return fHost; }

private:
    BString fUrl;
    BString fPath;
    BString fHost;
};

#endif
