#ifndef _MOCK_URL_H
#define _MOCK_URL_H

#include "String.h"

class BUrl {
public:
    BUrl(const char* url, bool parse = false) : fUrl(url) {
        (void)parse;
        const char* str = fUrl.String();
        const char* authorityStart = str;

        // Skip protocol
        const char* protocolEnd = strstr(str, "://");
        if (protocolEnd) {
            authorityStart = protocolEnd + 3;
        }

        // Find end of authority (start of path, query, or fragment)
        const char* authorityEnd = strpbrk(authorityStart, "/?#");
        int32 authorityLen;
        if (authorityEnd) {
            authorityLen = authorityEnd - authorityStart;
        } else {
            authorityLen = strlen(authorityStart);
        }

        // Extract authority section to process user:pass and port
        BString authority;
        authority.SetTo(authorityStart, authorityLen);

        const char* authStr = authority.String();
        const char* hostStart = authStr;

        // Check for user info (user:pass@host)
        const char* atSign = strrchr(authStr, '@');
        if (atSign) {
            hostStart = atSign + 1;
        }

        // Check for port (host:port), but be careful with IPv6 [::1]:80
        // For simple mock, assuming IPv4/Hostname for now as per AdBlock requirements
        const char* colon = strchr(hostStart, ':');
        if (colon) {
            fHost.SetTo(hostStart, colon - hostStart);
        } else {
            fHost = hostStart;
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
