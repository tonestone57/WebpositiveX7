
#include <stdio.h>
#include <map>
#include <string>
#include <vector>
#include <assert.h>
#include <iostream>

#include "MockFileSystem.h"
#include "String.h"
#include "File.h"
#include "Path.h"
#include "Directory.h"
#include "FindDirectory.h"
#include "Entry.h"
#include "Message.h"
#include "Autolock.h"
#include "Alert.h"
#include "Catalog.h"
#include "Locale.h"

// Define MockFileSystem statics
std::map<std::string, MockEntryData> MockFileSystem::sEntries;
long MockFileSystem::sGetNextEntryCount = 0;
long MockFileSystem::sOpenCount = 0;
long MockFileSystem::sReadAttrCount = 0;

// Track created directories
std::vector<std::string> sCreatedDirectories;

status_t create_directory(const char* path, mode_t mode) {
    if (path) {
        sCreatedDirectories.push_back(path);
        MockEntryData data;
        data.name = path; // Simplified
        data.path = path;
        data.isDirectory = true;
        MockFileSystem::AddEntry(path, data);
    }
    return B_OK;
}

#include "NodeInfo.h"

// Stub for BRoster
#include "Roster.h"
BRoster* be_roster = NULL;

#define _NODE_INFO_H
#include "../bookmarks/BookmarkManager.cpp"

// BFile::content static definition
std::string BFile::content = "";

// find_directory stub
status_t find_directory(directory_which which, BPath* path) {
    path->SetTo("/boot/home/config/settings");
    return B_OK;
}


int main() {
    printf("Testing BookmarkManager Import...\n");

    // Setup base path
    BPath basePath;
    BookmarkManager::GetBookmarkPath(basePath);
    // /boot/home/config/settings/WebPositive/Bookmarks

    // Setup Test Data
    // <DT><H3>EmptyFolder</H3> (followed by DT)
    // <DT><H3>NextFolder</H3>
    // <DT><H3>LastEmptyFolder</H3> (followed by /DL)
    std::string testHtml =
        "<!DOCTYPE NETSCAPE-Bookmark-file-1>\n"
        "<DL><p>\n"
        "<DT><H3>EmptyFolder</H3>\n"
        "<DT><H3>NextFolder</H3>\n"
        "<DL><p>\n"
        "<DT><A HREF=\"http://example.com\">Example</A>\n"
        "</DL><p>\n"
        "<DT><H3>LastEmptyFolder</H3>\n"
        "</DL><p>\n";

    // Set mock content for BFile to read
    BFile::content = testHtml;

    // Run Import
    BookmarkManager::ImportBookmarks(BPath("/tmp/bookmarks.html"));

    // Check results
    bool foundEmpty = false;
    bool foundLast = false;

    for (const auto& dir : sCreatedDirectories) {
        printf("Created: %s\n", dir.c_str());
        if (dir.find("EmptyFolder") != std::string::npos) foundEmpty = true;
        if (dir.find("LastEmptyFolder") != std::string::npos) foundLast = true;
    }

    if (!foundEmpty) {
        printf("FAILURE: EmptyFolder was not created.\n");
    } else {
        printf("SUCCESS: EmptyFolder created.\n");
    }

    if (!foundLast) {
        printf("FAILURE: LastEmptyFolder was not created.\n");
    } else {
        printf("SUCCESS: LastEmptyFolder created.\n");
    }

    if (!foundEmpty || !foundLast) return 1;

    return 0;
}
