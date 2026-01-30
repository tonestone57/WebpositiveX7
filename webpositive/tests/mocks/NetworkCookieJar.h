#ifndef _MOCK_NETWORK_COOKIE_JAR_H
#define _MOCK_NETWORK_COOKIE_JAR_H
#include <vector>
#include "NetworkCookie.h"

namespace BPrivate {
namespace Network {

class BNetworkCookieJar {
public:
    std::vector<BNetworkCookie> fCookies;

    class Iterator {
    public:
        Iterator(const std::vector<BNetworkCookie>& cookies) : fCookies(cookies), index(0) {}
        const BNetworkCookie* Next() {
            if (index < fCookies.size()) return &fCookies[index++];
            return NULL;
        }
    private:
        const std::vector<BNetworkCookie>& fCookies;
        size_t index;
    };

    Iterator GetIterator() { return Iterator(fCookies); }
    void AddCookie(const BNetworkCookie& c) { fCookies.push_back(c); }
};

} // namespace Network
} // namespace BPrivate
#endif
