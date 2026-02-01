#ifndef _ENTRY_H
#define _ENTRY_H
#include "SupportDefs.h"
#include "Path.h"
#include "MockFileSystem.h"
#include "EntryRef.h"
#include <string>
#include <string.h>
#include <map>

class BEntry {
public:
    std::string fName;
    std::string fPath;
    bool fExists;
    std::map<std::string, std::string> fAttributes;

    BEntry(const char* path, bool traverse = false) : fExists(false) {
        if (path) {
            fPath = path;
            // extract name
            const char* lastSlash = strrchr(path, '/');
            if (lastSlash) fName = lastSlash + 1;
            else fName = path;

            MockEntryData data;
            if (MockFileSystem::GetEntry(fPath, &data)) {
                fExists = true;
                fAttributes = data.attributes;
            }
        }
    }
    BEntry(const BPath& path) : BEntry(path.Path()) {}

    // Constructor from entry_ref
    BEntry(const entry_ref* ref, bool traverse = false) : fExists(false) {
        if (ref) {
            fPath = ref->path;
             // extract name
            size_t lastSlash = fPath.rfind('/');
            if (lastSlash != std::string::npos) fName = fPath.substr(lastSlash + 1);
            else fName = fPath;

            MockEntryData data;
            if (MockFileSystem::GetEntry(fPath, &data)) {
                fExists = true;
                fAttributes = data.attributes;
            }
        }
    }

    status_t InitCheck() { return B_OK; }
    status_t Rename(const char* path, bool clobber = false) { return B_OK; }
    status_t Remove() { return B_OK; }
    BEntry() : fExists(false) {}
    status_t SetTo(const char* path, bool traverse = false) {
        *this = BEntry(path, traverse);
        return B_OK;
    }
    status_t GetPath(BPath* path) const {
        if (path) path->SetTo(fPath.c_str());
        return B_OK;
    }
    bool Exists() const { return fExists; }
    status_t GetRef(entry_ref* ref) const {
        if (ref) ref->path = fPath;
        return B_OK;
    }
    status_t GetName(char* buffer) {
        if (fName.length() >= B_FILE_NAME_LENGTH) {
            strncpy(buffer, fName.c_str(), B_FILE_NAME_LENGTH - 1);
            buffer[B_FILE_NAME_LENGTH - 1] = 0;
        } else {
            strcpy(buffer, fName.c_str());
        }
        return B_OK;
    }
    bool IsDirectory() const {
         MockEntryData data;
         if (MockFileSystem::GetEntry(fPath, &data)) {
             return data.isDirectory;
         }
         return false;
    }
    status_t GetSize(off_t* size) const {
        MockEntryData data;
         if (MockFileSystem::GetEntry(fPath, &data)) {
             *size = data.content.length();
             return B_OK;
         }
        *size = 0;
        return B_OK;
    }
    void Unset() {
        fPath = "";
        fName = "";
        fExists = false;
        fAttributes.clear();
    }

    // Test helpers
    void SetExists(bool exists) { fExists = exists; }
    void SetName(const char* name) { fName = name; }
    void SetAttribute(const std::string& key, const std::string& value) {
        fAttributes[key] = value;
        // Update mock FS
        MockEntryData data;
        if (MockFileSystem::GetEntry(fPath, &data)) {
            data.attributes[key] = value;
            MockFileSystem::AddEntry(fPath, data);
        }
    }
};

inline status_t get_ref_for_path(const char* path, entry_ref* ref) {
    if (ref && path) ref->path = path;
    return B_OK;
}
#endif
