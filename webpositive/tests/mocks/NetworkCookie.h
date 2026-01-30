#ifndef _MOCK_NETWORK_COOKIE_H
#define _MOCK_NETWORK_COOKIE_H
#include "String.h"
#include <time.h>

namespace BPrivate {
namespace Network {

class BNetworkCookie {
public:
    BNetworkCookie() : fExpiration(0), fSecure(false), fHttpOnly(false), fHostOnly(false) {}
    BNetworkCookie(const char* name, const char* value) : fName(name), fValue(value), fExpiration(0), fSecure(false), fHttpOnly(false), fHostOnly(false) {}

    BString Domain() const { return fDomain; }
    BString Path() const { return fPath; }
    BString Name() const { return fName; }
    BString Value() const { return fValue; }
    time_t ExpirationDate() const { return fExpiration; }
    bool Secure() const { return fSecure; }
    bool HttpOnly() const { return fHttpOnly; }
    bool IsHostOnly() const { return fHostOnly; }

    void SetDomain(const char* d) { fDomain = d; }
    void SetPath(const char* p) { fPath = p; }
    void SetName(const char* n) { fName = n; }
    void SetValue(const char* v) { fValue = v; }
    void SetExpirationDate(time_t t) { fExpiration = t; }
    void SetSecure(bool b) { fSecure = b; }
    void SetHttpOnly(bool b) { fHttpOnly = b; }
    void SetHostOnly(bool b) { fHostOnly = b; }

private:
    BString fDomain;
    BString fPath;
    BString fName;
    BString fValue;
    time_t fExpiration;
    bool fSecure;
    bool fHttpOnly;
    bool fHostOnly;
};

} // namespace Network
} // namespace BPrivate
#endif
