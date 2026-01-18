#include <stdio.h>
#include <vector>
#include <string>
#include <map>
#include <assert.h>

// Mocks must be included or picked up from search path
#include "mocks/NetworkCookie.h"
#include "mocks/NetworkCookieJar.h"
#include "mocks/File.h"

// The class we test
#include "../Sync.h"

void TestExportCookies() {
    printf("TestExportCookies...\n");
    BFile::sFileSystem.clear();

    BPrivate::Network::BNetworkCookieJar jar;
    BPrivate::Network::BNetworkCookie cookie("session_id", "12345");
    cookie.SetDomain("example.com");
    cookie.SetPath("/");
    cookie.SetSecure(true);
    cookie.SetExpirationDate(1700000000);

    jar.AddCookie(cookie);

    BPath path("/tmp/cookies.txt");
    status_t result = Sync::ExportCookies(path, jar);

    if (result != B_OK) {
        printf("FAILED: ExportCookies returned %d\n", result);
        assert(result == B_OK);
    }

    if (BFile::sFileSystem.count("/tmp/cookies.txt") != 1) {
        printf("FAILED: File was not created\n");
        assert(false);
    }

    std::string content = BFile::sFileSystem["/tmp/cookies.txt"];
    // Verify content contains cookie data
    // Format: domain flag path secure expiration name value
    // example.com TRUE / TRUE 1700000000 session_id 12345

    // Note: BString implementation in mocks might behave slightly differently than real one if not full?
    // But we use real logic in Sync.cpp with mocked BString.

    // Check for presence of fields
    assert(content.find("example.com") != std::string::npos);
    assert(content.find("session_id") != std::string::npos);
    assert(content.find("12345") != std::string::npos);
    assert(content.find("TRUE") != std::string::npos); // Secure

    printf("TestExportCookies passed.\n");
}

void TestImportCookies() {
    printf("TestImportCookies...\n");
    BFile::sFileSystem.clear();

    // Netscape cookie format
    // domain flag path secure expiration name value
    std::string content =
        "# Netscape HTTP Cookie File\n"
        "example.com\tFALSE\t/\tTRUE\t1700000000\tsession_id\t12345\n";

    BFile::sFileSystem["/tmp/cookies_import.txt"] = content;

    BPrivate::Network::BNetworkCookieJar jar;
    BPath path("/tmp/cookies_import.txt");

    status_t result = Sync::ImportCookies(path, jar);

    assert(result == B_OK);
    const std::vector<BPrivate::Network::BNetworkCookie>& cookies = jar.GetCookies();
    assert(cookies.size() == 1);
    assert(cookies[0].Name() == "session_id");
    assert(cookies[0].Value() == "12345");
    assert(cookies[0].Domain() == "example.com");
    assert(cookies[0].Secure() == true);

    printf("TestImportCookies passed.\n");
}

void TestExportProfile() {
    printf("TestExportProfile...\n");
    BFile::sFileSystem.clear();

    BPrivate::Network::BNetworkCookieJar jar;
    BPath folder("/tmp/profile");

    status_t result = Sync::ExportProfile(folder, jar);

    assert(result == B_OK);
    // Should have created bookmarks.html, history.csv, cookies.txt
    assert(BFile::sFileSystem.count("/tmp/profile/bookmarks.html") == 1);
    assert(BFile::sFileSystem.count("/tmp/profile/history.csv") == 1);
    assert(BFile::sFileSystem.count("/tmp/profile/cookies.txt") == 1);

    // Verify mocked content
    assert(BFile::sFileSystem["/tmp/profile/bookmarks.html"] == "MOCKED BOOKMARKS");
    assert(BFile::sFileSystem["/tmp/profile/history.csv"] == "MOCKED HISTORY");

    printf("TestExportProfile passed.\n");
}

int main() {
    printf("Running SyncTest...\n");
    TestExportCookies();
    TestImportCookies();
    TestExportProfile();
    printf("All SyncTest passed!\n");
    return 0;
}
