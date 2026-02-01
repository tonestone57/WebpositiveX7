#ifndef _MOCK_DIRECTORY_H
#define _MOCK_DIRECTORY_H
#include "SupportDefs.h"
#include "Path.h"
#include "Entry.h"
#include <sys/types.h>
#include <map>
#include <string>

class BDirectory {
public:
    std::map<std::string, BEntry> fEntryMap;
    std::map<std::string, BEntry>::iterator fIter;

    BDirectory(const BEntry* entry) { fIter = fEntryMap.begin(); }
    BDirectory(const char* path) { fIter = fEntryMap.begin(); }
    status_t InitCheck() { return B_OK; }

    void Rewind() { fIter = fEntryMap.begin(); }

    status_t GetNextEntry(BEntry* entry, bool traverse = false) {
        if (fIter != fEntryMap.end()) {
            if (entry) *entry = fIter->second;
            fIter++;
            return B_OK;
        }
        return B_ENTRY_NOT_FOUND;
    }

    status_t FindEntry(const char* path, BEntry* entry, bool traverse = false) {
        auto it = fEntryMap.find(path);
        if (it != fEntryMap.end()) {
            if (entry) *entry = it->second;
            return B_OK;
        }
        return B_ENTRY_NOT_FOUND;
    }

    // Test Helper
    void AddEntry(const BEntry& entry) {
        fEntryMap[entry.fName] = entry;
    }
    void Clear() {
        fEntryMap.clear();
        fIter = fEntryMap.begin();
    }
};

status_t create_directory(const char* path, mode_t mode);
#endif
