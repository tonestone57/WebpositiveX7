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
// MockFileSystem static members need definition if not linked
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

// Mock create_directory
status_t create_directory(const char* path, mode_t mode) { return B_OK; }

#define B_TRANSLATION_CONTEXT "Benchmark"
#include "../bookmarks/BookmarkManager.cpp"

void SetupBookmarks(int numEntries) {
    MockFileSystem::Reset();

    // Create entries in MockFileSystem
    // Note: The MockDirectory implementation scans MockFileSystem::sEntries to find children.
    // So we just need to add entries with the correct path prefix.

    std::string baseDir = "/boot/home/config/settings/WebPositive/Bookmarks";

    for (int i = 0; i < numEntries; ++i) {
        char name[64];
        sprintf(name, "bookmark_%d", i);
        BString fullPath = baseDir.c_str();
        fullPath << "/" << name;

        MockEntryData data;
        data.name = name;
        data.path = fullPath.String();
        data.isDirectory = false;
        data.attributes["META:url"] = "http://example.com";
        data.attributes["META:title"] = name;

        MockFileSystem::AddEntry(fullPath.String(), data);
    }
}

void RunBenchmark(int numEntries) {
    SetupBookmarks(numEntries);

    // Reset counters before running the target function
    MockFileSystem::sOpenCount = 0;

    BPath exportPath("/tmp/bookmarks.html");

    struct timeval start, end;
    gettimeofday(&start, NULL);

    status_t result = BookmarkManager::ExportBookmarks(exportPath);

    gettimeofday(&end, NULL);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    printf("Entries: %d\n", numEntries);
    printf("Result: %d\n", result);
    printf("Time: %.6f s\n", elapsed);
    printf("OpenCount: %ld\n", MockFileSystem::sOpenCount);
    printf("----------------------------------------\n");
}

int main() {
    printf("Running BookmarkExportBenchmark...\n");
    RunBenchmark(100);
    RunBenchmark(1000);
    return 0;
}
