
#include <stdio.h>
#include <assert.h>
#include <string>
#include <vector>
#include <map>

// Mocks
#include "NetworkCookie.h"
#include "NetworkCookieJar.h"
#include "File.h"
#include "Path.h"
#include "String.h"
#include "MockFileSystem.h"

// Define BFile::content
std::string BFile::content = "";

// Define MockFileSystem statics
std::map<std::string, MockEntryData> MockFileSystem::sEntries;
long MockFileSystem::sGetNextEntryCount = 0;
long MockFileSystem::sOpenCount = 0;
long MockFileSystem::sReadAttrCount = 0;

// Stubs
status_t create_directory(const char* path, mode_t mode) { return B_OK; }

#include "../BrowsingHistory.h"
status_t BrowsingHistory::ExportHistory(const BPath& path) { return B_OK; }
status_t BrowsingHistory::ImportHistory(const BPath& path) { return B_OK; }

// BrowsingHistory destructor is virtual, so we might need more if we don't link it.
// But we are only using static methods from it in Sync.cpp (ExportHistory/ImportHistory).
// Sync.cpp includes BrowsingHistory.h.
// Since we implement the static methods here, we don't need to link BrowsingHistory.cpp.
// However, BrowsingHistory.h defines a class. If Sync.cpp doesn't instantiate it, we are fine.
// Sync.cpp only calls static methods.

// We also need BookmarkManager stubs because Sync.cpp uses them.
#include "../bookmarks/BookmarkManager.h"
status_t BookmarkManager::ExportBookmarks(const BPath& path) { return B_OK; }
status_t BookmarkManager::ImportBookmarks(const BPath& path) { return B_OK; }

// Include source
#include "../Sync.cpp"

int main() {
    printf("Testing ImportCookies with empty value...\n");

    // "domain flag path secure expiration name value"
    // Tab separated.
    // Case 1: Normal
    std::string data = "example.com\tTRUE\t/\tFALSE\t0\tTestCookie\tTestValue\n";
    // Case 2: Empty Value. Netscape format usually ends with newline.
    // If value is empty: domain...name\t\n
    data += "example.com\tTRUE\t/\tFALSE\t0\tEmptyCookie\t\n";

    BFile::content = data;

    BPrivate::Network::BNetworkCookieJar jar;
    Sync::ImportCookies(BPath("/tmp/cookies.txt"), jar);

    bool foundNormal = false;
    bool foundEmpty = false;

    BPrivate::Network::BNetworkCookieJar::Iterator it = jar.GetIterator();
    const BPrivate::Network::BNetworkCookie* c;
    while ((c = it.Next()) != NULL) {
        printf("Cookie: Name='%s', Value='%s'\n", c->Name().String(), c->Value().String());
        if (c->Name() == "TestCookie" && c->Value() == "TestValue") foundNormal = true;
        if (c->Name() == "EmptyCookie" && c->Value() == "") foundEmpty = true;
    }

    if (!foundNormal) {
        printf("FAILURE: Normal cookie not imported.\n");
        return 1;
    }

    if (!foundEmpty) {
        printf("FAILURE: Empty value cookie not imported.\n");
        return 1;
    }

    printf("SUCCESS: Both cookies imported.\n");
    return 0;
}
