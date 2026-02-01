#include <assert.h>
#include <stdio.h>
#include <string.h> // Ensure system headers are found first or available
#include <stdlib.h> // Ensure system headers are found first or available
#include <iostream>
#include <string>

// Include mocks explicitly to define static members/implementations.
// Note: These must be included before any source files that might include the real system headers
// to ensure the mocks take precedence in this test environment.
#include "File.h"
#include "Directory.h"
#include "NetworkCookieJar.h"
#include "BookmarkManager.h"
#include "BrowsingHistory.h"

// Define static content for BFile mock
std::string BFile::content = "";

// Define create_directory mock
status_t create_directory(const char* path, mode_t mode) { return B_OK; }

// Include the source file under test.
// Prevent system headers from being included by Sync.cpp, as we want to use the mocks.
#define _FILE_H
#define _DIRECTORY_H
#define _PATH_H
#define _FIND_DIRECTORY_H
#define _B_STRING_H
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
    c2.SetHostOnly(true);
    c2.SetExpirationDate(1800000000);
    jar.AddCookie(c2);

    BPath path("/tmp/cookies.txt");
    BFile::content = ""; // Clear buffer

    status_t status = Sync::ExportCookies(path, jar);

    assert(status == B_OK);

    // Verify content
    // Header
    assert(BFile::content.find("# Netscape HTTP Cookie File") != std::string::npos);

    // Cookie 1
    std::string line1 = "#HttpOnly_example.com\tTRUE\t/\tTRUE\t1700000000\tsession\t12345";
    if (BFile::content.find(line1) == std::string::npos) {
        printf("FAILED: Could not find line1\n");
        printf("Content:\n%s\n", BFile::content.c_str());
        return 1;
    }

    // Cookie 2
    std::string line2 = "test.org\tFALSE\t/foo\tFALSE\t1800000000\ttracking\tabc";
    if (BFile::content.find(line2) == std::string::npos) {
        printf("FAILED: Could not find line2\n");
        return 1;
    }

    printf("SyncTest Passed!\n");
    return 0;
}
