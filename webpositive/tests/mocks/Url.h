#ifndef URL_H
#define URL_H

#include "String.h"

class BUrl {
    BString fUrl;
    BString fProtocol;
    BString fHost;
    int fPort;

public:
    BUrl(const BString& url) : fUrl(url), fPort(-1) {
        // Simple parsing
        int32 protoEnd = url.FindFirst("://");
        if (protoEnd > 0) {
            url.CopyInto(fProtocol, 0, protoEnd);
            int32 hostStart = protoEnd + 3;
            int32 hostEnd = url.FindFirst("/");
            if (hostEnd < 0) hostEnd = url.Length();

            // Check for port
            int32 portStart = -1;
            for (int i = hostStart; i < hostEnd; i++) {
                if (url[i] == ':') {
                    portStart = i;
                    break;
                }
            }

            if (portStart > 0) {
                url.CopyInto(fHost, hostStart, portStart - hostStart);
                // Parse port...
            } else {
                url.CopyInto(fHost, hostStart, hostEnd - hostStart);
            }
        } else {
             fHost = url; // Assume just host if no protocol
        }
    }

    bool IsValid() const { return fUrl.Length() > 0; }
    BString Host() const { return fHost; }
    BString Protocol() const { return fProtocol; }
    int Port() const { return fPort; }
};

#endif
