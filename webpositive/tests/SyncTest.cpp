#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// --- Mocks ---

typedef int32_t status_t;
const status_t B_OK = 0;
const status_t B_ERROR = -1;
const status_t B_NO_MEMORY = -2;
const status_t B_IO_ERROR = -3;

// Mock BString
class BString {
public:
    std::string s;
    BString() {}
    BString(const char* str) : s(str ? str : "") {}

    const char* String() const { return s.c_str(); }
    int32_t Length() const { return (int32_t)s.length(); }
    void Truncate(int32_t len) { s.resize(len); }

    BString& operator<<(const char* str) { s += str; return *this; }
    BString& operator<<(const BString& str) { s += str.s; return *this; }
    BString& operator<<(int val) { s += std::to_string(val); return *this; }
    BString& operator<<(long val) { s += std::to_string(val); return *this; }
};

// Mock BPath
class BPath {
public:
    BPath(const char* p) : path(p) {}
    const char* Path() const { return path.c_str(); }
    void Append(const char* p) { path += "/"; path += p; }
private:
    std::string path;
};

// Mock BFile
class BFile {
public:
    BFile(const char* path, uint32_t mode) {}
    status_t InitCheck() { return B_OK; }
    ssize_t Write(const void* buffer, size_t size) {
        content.append((const char*)buffer, size);
        return size;
    }

    static std::string content;
};
std::string BFile::content = "";

enum {
    B_READ_ONLY = 1,
    B_WRITE_ONLY = 2,
    B_CREATE_FILE = 4,
    B_ERASE_FILE = 8
};

// Mock NetworkCookie
namespace BPrivate {
namespace Network {

class BNetworkCookie {
public:
    BNetworkCookie() {}

    BString Domain() const { return fDomain; }
    BString Path() const { return fPath; }
    BString Name() const { return fName; }
    BString Value() const { return fValue; }
    time_t ExpirationDate() const { return fExpiration; }
    bool Secure() const { return fSecure; }
    bool HttpOnly() const { return fHttpOnly; }
    bool IsHostOnly() const { return fHostOnly; }

    // Setters for test setup
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

// Stub Sync class to allow including Sync.cpp logic without linking
// We will manually define the method we want to test by including a modified version or just coping it?
// Including Sync.cpp directly is hard because of other dependencies (BookmarkManager, BrowsingHistory).
// So I will COPY the ExportCookies implementation here for testing.
// Ideally I would define the mocks in headers and include Sync.cpp, but header conflict is an issue.

// Wait, the task is to verify the change I made.
// I made the change in Sync.cpp.
// If I copy the code here, I am not testing the file.
// I must include Sync.cpp.
// I can define stubs for BookmarkManager and BrowsingHistory.

namespace BPrivate {
    class BBookmarkNode;
}
class BookmarkManager {
public:
    static status_t ExportBookmarks(const BPath& path) { return B_OK; }
    static status_t ImportBookmarks(const BPath& path) { return B_OK; }
};

class BrowsingHistory {
public:
    static status_t ExportHistory(const BPath& path) { return B_OK; }
    static status_t ImportHistory(const BPath& path) { return B_OK; }
};

status_t create_directory(const char* path, mode_t mode) { return B_OK; }

// Dummy defines to satisfy Sync.h includes if any
#define SYNC_H
// We need to define Sync class structure so we can include the cpp file which defines the methods.
class Sync {
public:
	static status_t ExportProfile(const BPath& folder,
		BPrivate::Network::BNetworkCookieJar& cookieJar);
	static status_t ImportProfile(const BPath& folder,
		BPrivate::Network::BNetworkCookieJar& cookieJar);

	static status_t ExportCookies(const BPath& path,
		BPrivate::Network::BNetworkCookieJar& cookieJar);
	static status_t ImportCookies(const BPath& path,
		BPrivate::Network::BNetworkCookieJar& cookieJar);
};

// Prevent including real headers
#define _DIRECTORY_H
#define _FILE_H
#define _NETWORK_COOKIE_H
#define _NETWORK_COOKIE_JAR_H
#define _STRING_H
#define _SUPPORT_DEFS_H
#define BOOKMARK_MANAGER_H
#define BROWSING_HISTORY_H

// Now include the source
#include "../Sync.cpp"

int main() {
    printf("Running SyncTest...\n");

    BPrivate::Network::BNetworkCookieJar jar;

    // Add some cookies
    BPrivate::Network::BNetworkCookie c1;
    c1.SetDomain("example.com");
    c1.SetPath("/");
    c1.SetName("session");
    c1.SetValue("12345");
    c1.SetSecure(true);
    c1.SetHttpOnly(true);
    c1.SetHostOnly(false);
    c1.SetExpirationDate(1700000000);
    jar.AddCookie(c1);

    BPrivate::Network::BNetworkCookie c2;
    c2.SetDomain("test.org");
    c2.SetPath("/foo");
    c2.SetName("tracking");
    c2.SetValue("abc");
    c2.SetSecure(false);
    c2.SetHttpOnly(false);
    c2.SetHostOnly(true); // Should result in FALSE in flag column?
                          // Logic: line << (!cookie->IsHostOnly() ? "TRUE" : "FALSE")
                          // If IsHostOnly is true, it outputs FALSE (include subdomains is false).
    c2.SetExpirationDate(1800000000);
    jar.AddCookie(c2);

    BPath path("/tmp/cookies.txt");
    BFile::content = ""; // Clear buffer

    status_t status = Sync::ExportCookies(path, jar);

    assert(status == B_OK);

    printf("Exported content:\n%s\n", BFile::content.c_str());

    // Verify content
    // Header
    assert(BFile::content.find("# Netscape HTTP Cookie File") != std::string::npos);

    // Cookie 1
    // #HttpOnly_example.com	TRUE	/	TRUE	1700000000	session	12345
    std::string line1 = "#HttpOnly_example.com\tTRUE\t/\tTRUE\t1700000000\tsession\t12345";
    assert(BFile::content.find(line1) != std::string::npos);

    // Cookie 2
    // test.org	FALSE	/foo	FALSE	1800000000	tracking	abc
    std::string line2 = "test.org\tFALSE\t/foo\tFALSE\t1800000000\ttracking\tabc";
    assert(BFile::content.find(line2) != std::string::npos);

    printf("SyncTest Passed!\n");
    return 0;
}
