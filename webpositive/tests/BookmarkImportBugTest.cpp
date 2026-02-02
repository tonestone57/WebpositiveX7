#include <stdio.h>
#include <map>
#include <string>
#include <vector>

#include "MockFileSystem.h"

#include <assert.h>
#include <iostream>

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

// Include source
#define _ALERT_H
#define _DIRECTORY_H
#define _FILE_H
#define _FIND_DIRECTORY_H
#define _PATH_H
#define _ROSTER_H
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
    // <DT><H3>EmptyFolder</H3> (No DL, immediately followed by another folder)
    // <DT><H3>NextFolder</H3>
    std::string testHtml =
        "<!DOCTYPE NETSCAPE-Bookmark-file-1>\n"
        "<DT><H3>EmptyFolder</H3>\n"
        "<DT><H3>NextFolder</H3>\n"
        "<DL><p>\n"
        "<DT><A HREF=\"http://example.com\">Example</A>\n"
        "</DL><p>\n";

    // Set mock content for BFile to read
    BFile::content = testHtml;

    // Run Import
    BookmarkManager::ImportBookmarks(BPath("/tmp/bookmarks.html"));

    // Check results
    bool foundEmpty = false;
    bool foundNext = false;

    for (const auto& dir : sCreatedDirectories) {
        printf("Created: %s\n", dir.c_str());
        if (dir.find("EmptyFolder") != std::string::npos) foundEmpty = true;
        if (dir.find("NextFolder") != std::string::npos) foundNext = true;
    }

    if (!foundEmpty) {
        printf("FAILURE: EmptyFolder was not created.\n");
    } else {
        printf("SUCCESS: EmptyFolder created.\n");
    }

    if (!foundNext) {
        printf("FAILURE: NextFolder was not created.\n");
    }

    if (!foundEmpty || !foundNext) return 1;

    return 0;
}
