#ifndef _MOCK_DIRECTORY_H
#define _MOCK_DIRECTORY_H
#include "SupportDefs.h"
#include "Path.h"
#include "Entry.h"
#include "MockFileSystem.h"
#include "Volume.h"
#include <sys/types.h>
#include <map>
#include <string>
#include <vector>

class BDirectory {
public:
    std::string fPath;
    std::vector<std::string> fChildren; // Cache of children names
    size_t fIterIndex;

    BDirectory(const BEntry* entry) {
        fIterIndex = 0;
        if (entry) {
            fPath = entry->fPath;
            _PopulateChildren();
        }
    }
    BDirectory(const char* path) {
        fIterIndex = 0;
        if (path) {
            fPath = path;
            _PopulateChildren();
        }
    }
    status_t InitCheck() { return B_OK; }

    void Rewind() { fIterIndex = 0; }

    status_t GetNextEntry(BEntry* entry, bool traverse = false) {
        MockFileSystem::sGetNextEntryCount++;
        if (fIterIndex < fChildren.size()) {
            std::string childPath = fChildren[fIterIndex++];
            if (entry) entry->SetTo(childPath.c_str());
            return B_OK;
        }
        return B_ENTRY_NOT_FOUND;
    }

    status_t FindEntry(const char* path, BEntry* entry, bool traverse = false) {
        std::string fullPath = fPath + "/" + path;
        MockEntryData data;
        if (MockFileSystem::GetEntry(fullPath, &data)) {
            if (entry) entry->SetTo(fullPath.c_str());
            return B_OK;
        }
        return B_ENTRY_NOT_FOUND;
    }

    status_t GetEntry(BEntry* entry) {
        if (entry) return entry->SetTo(fPath.c_str());
        return B_OK;
    }

    status_t GetVolume(BVolume* vol) const {
        return B_OK;
    }

    // Test Helper
    void AddEntry(const BEntry& entry) {
        std::string fullPath;
        if (fPath.empty()) fullPath = entry.fName;
        else fullPath = fPath + "/" + entry.fName;

        MockEntryData data;
        data.name = entry.fName;
        data.path = fullPath;
        data.attributes = entry.fAttributes;
        data.isDirectory = false;

        MockFileSystem::AddEntry(fullPath, data);
        _PopulateChildren();
    }

    // Helper to add directory
    void AddDirectory(const char* name) {
        std::string fullPath = fPath + "/" + name;
        MockEntryData data;
        data.name = name;
        data.path = fullPath;
        data.isDirectory = true;
        MockFileSystem::AddEntry(fullPath, data);
        _PopulateChildren();
    }

    void Clear() {
        fChildren.clear();
        fIterIndex = 0;
    }

private:
    void _PopulateChildren() {
        fChildren.clear();
        if (fPath.empty()) return;

        // Brute force scan of sEntries
        size_t prefixLen = fPath.length();
        if (fPath.back() != '/') prefixLen++;

        for (auto const& [path, data] : MockFileSystem::sEntries) {
            if (path.length() > prefixLen && path.compare(0, fPath.length(), fPath) == 0) {
                 if (fPath.back() != '/' && path[fPath.length()] != '/') continue;

                 // It is a descendant. Is it a direct child?
                 size_t nextSlash = path.find('/', prefixLen);
                 if (nextSlash == std::string::npos) {
                     fChildren.push_back(path);
                 }
            }
        }
    }
};

status_t create_directory(const char* path, mode_t mode);
#endif
