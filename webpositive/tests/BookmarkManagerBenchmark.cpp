#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <sys/time.h>
#include <map>

// Mock includes
#include "mocks/SupportDefs.h"
#include "mocks/Path.h"
#include "mocks/Entry.h"
#include "mocks/Node.h"
#include "mocks/File.h"
#include "mocks/Directory.h"
#include "mocks/String.h"
#include "mocks/Message.h"
#include "mocks/Invoker.h"
#include "mocks/Alert.h"
#include "mocks/Roster.h"
#include "mocks/FindDirectory.h"
#include "mocks/NodeInfo.h"
#include "mocks/Catalog.h"

// Define static members for mocks
std::string BFile::content = "";
std::map<std::string, MockEntryData> MockFileSystem::sEntries;
long MockFileSystem::sGetNextEntryCount = 0;
long MockFileSystem::sOpenCount = 0;
long MockFileSystem::sReadAttrCount = 0;

// Implement find_directory
status_t find_directory(directory_which which, BPath* path) {
    if (path) path->SetTo("/boot/home/config/settings");
    return B_OK;
}

// Implement be_roster
BRoster* be_roster = new BRoster();

// Mock create_directory (needed by BookmarkManager)
status_t create_directory(const char* path, mode_t mode) { return B_OK; }

// Include source
// We need to define B_TRANSLATION_CONTEXT to avoid errors if it's redefined
#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Benchmark"
#include "../bookmarks/BookmarkManager.cpp"

// Benchmark function
void RunBenchmark(int numEntries) {
    BDirectory dir("/boot/home/config/settings/WebPositive/Bookmarks");

    // Create entries
    for (int i = 0; i < numEntries; ++i) {
        char name[64];
        sprintf(name, "bookmark_%d", i);
        BEntry entry(name);

        entry.SetAttribute("META:url", "http://example.com/foo");
        entry.SetAttribute("META:title", name);

        dir.AddEntry(entry);
    }

    // Add the target bookmark at the END (worst case for O(N))
    BEntry targetEntry("target_bookmark");
    targetEntry.SetAttribute("META:url", "http://target.com");
    dir.AddEntry(targetEntry);

    // Test CheckBookmarkExists
    BString name("target_bookmark");
    BString url("http://target.com");

    struct timeval start, end;
    gettimeofday(&start, NULL);

    // Repeat the check multiple times to get measurable time for small N
    int iterations = 1000;
    bool result = false;
    for (int i=0; i<iterations; i++) {
        dir.Rewind(); // Important for O(N) scan
        result = BookmarkManager::CheckBookmarkExists(dir, name, url);
    }

    gettimeofday(&end, NULL);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    if (result) {
        printf("Found bookmark in %.6f seconds (Entries: %d, Iterations: %d)\n", elapsed, numEntries, iterations);
        printf("Average time per call: %.9f seconds\n", elapsed / iterations);
    } else {
        printf("FAILED to find bookmark! (%.6f seconds)\n", elapsed);
    }

    // Verify negative case: Bookmark exists but wrong URL
    BString wrongUrl("http://wrong.com");
    if (BookmarkManager::CheckBookmarkExists(dir, name, wrongUrl)) {
        printf("ERROR: Found bookmark with wrong URL!\n");
    }

    // Verify negative case: Bookmark does not exist
    BString missingName("non_existent");
    if (BookmarkManager::CheckBookmarkExists(dir, missingName, url)) {
        printf("ERROR: Found non-existent bookmark!\n");
    }
}

int main() {
    printf("Running BookmarkManager Benchmark...\n");
    RunBenchmark(100);
    RunBenchmark(1000);
    RunBenchmark(5000);
    return 0;
}
