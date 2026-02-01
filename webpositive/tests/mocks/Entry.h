#ifndef _ENTRY_H
#define _ENTRY_H
#include "SupportDefs.h"
#include "Path.h"
#include <string>
#include <string.h>
#include <map>

struct entry_ref {};

class BEntry {
public:
    std::string fName;
    bool fExists;
    std::map<std::string, std::string> fAttributes;

    BEntry(const char* path, bool traverse = false) : fExists(false) {
        if (path) fName = path;
    }
    BEntry(const BPath& path) : fExists(false) {
        if (path.Path()) fName = path.Path(); // Simplified
    }
    status_t InitCheck() { return B_OK; }
    status_t Rename(const char* path, bool clobber = false) { return B_OK; }
    status_t Remove() { return B_OK; }
    BEntry() : fExists(false) {}
    status_t SetTo(const char* path, bool traverse = false) {
        if (path) fName = path;
        return B_OK;
    }
    status_t GetPath(BPath* path) const { return B_OK; }
    bool Exists() const { return fExists; }
    status_t GetRef(entry_ref* ref) const { return B_OK; }
    status_t GetName(char* buffer) {
        if (fName.length() >= B_FILE_NAME_LENGTH) {
            strncpy(buffer, fName.c_str(), B_FILE_NAME_LENGTH - 1);
            buffer[B_FILE_NAME_LENGTH - 1] = 0;
        } else {
            strcpy(buffer, fName.c_str());
        }
        return B_OK;
    }
    bool IsDirectory() const { return false; }
    status_t GetSize(off_t* size) const { *size = 0; return B_OK; }
    void Unset() {}

    // Test helpers
    void SetExists(bool exists) { fExists = exists; }
    void SetName(const char* name) { fName = name; }
    void SetAttribute(const std::string& key, const std::string& value) {
        fAttributes[key] = value;
    }
};

inline status_t get_ref_for_path(const char* path, entry_ref* ref) { return B_OK; }
#endif
