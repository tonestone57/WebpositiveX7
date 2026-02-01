#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <sys/time.h>
#include <map>

// Mock includes
#include "mocks/SupportDefs.h"
#include "mocks/Path.h"
#include "mocks/MockFileSystem.cpp" // Include impl
#include "mocks/Entry.h"
#include "mocks/Node.h"
#include "mocks/File.h"
#include "mocks/Volume.h"
#include "mocks/Directory.h"
#include "mocks/Query.h"
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

// Implement find_directory
status_t find_directory(directory_which which, BPath* path) {
    if (path) path->SetTo("/boot/home/config/settings");
    return B_OK;
}

BRoster* be_roster = new BRoster();
status_t create_directory(const char* path, mode_t mode) { return B_OK; }

// Include source
#define B_TRANSLATION_CONTEXT "Benchmark"
#include "../bookmarks/BookmarkManager.cpp"

void PopulateRecursive(BDirectory& dir, int depth, int filesPerDir, int dirsPerDir) {
    if (depth == 0) return;

    for (int i = 0; i < filesPerDir; ++i) {
        char name[64];
        sprintf(name, "file_%d", i);
        BEntry entry(name);
        // Mark as bookmark
        entry.SetAttribute("META:url", "http://example.com");
        dir.AddEntry(entry);
    }

    // Add non-bookmark files
    for (int i = 0; i < filesPerDir; ++i) {
        char name[64];
        sprintf(name, "other_%d", i);
        BEntry entry(name);
        // No attribute
        dir.AddEntry(entry);
    }

    for (int i = 0; i < dirsPerDir; ++i) {
        char name[64];
        sprintf(name, "subdir_%d", i);
        dir.AddDirectory(name);

        // Open subdir to populate
        BEntry subdirEntry;
        dir.FindEntry(name, &subdirEntry);
        BDirectory subdir(&subdirEntry);
        PopulateRecursive(subdir, depth - 1, filesPerDir, dirsPerDir);
    }
}

void RunBenchmark() {
    MockFileSystem::Reset();

    // Root dir
    BDirectory rootDir("/boot/home/config/settings/WebPositive/Bookmarks");

    // Create a tree: depth 4, 10 files, 3 subdirs
    PopulateRecursive(rootDir, 4, 10, 3);

    printf("Total entries in FS: %lu\n", MockFileSystem::sEntries.size());

    // Reset counters
    MockFileSystem::sGetNextEntryCount = 0;
    MockFileSystem::sOpenCount = 0;
    MockFileSystem::sReadAttrCount = 0;

    BMessage message;
    uint32 addedCount = 0;

    rootDir.Rewind();

    struct timeval start, end;
    gettimeofday(&start, NULL);

    BookmarkManager::AddBookmarkURLsRecursively(rootDir, &message, addedCount);

    gettimeofday(&end, NULL);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    printf("AddBookmarkURLsRecursively finished in %.6f seconds\n", elapsed);
    printf("Bookmarks found: %d\n", (int)addedCount);
    printf("GetNextEntry calls: %ld\n", MockFileSystem::sGetNextEntryCount);
    printf("BFile Open calls: %ld\n", MockFileSystem::sOpenCount);
    printf("ReadAttr calls: %ld\n", MockFileSystem::sReadAttrCount);
}

int main() {
    printf("Running BookmarkQueryBenchmark...\n");
    RunBenchmark();
    return 0;
}
