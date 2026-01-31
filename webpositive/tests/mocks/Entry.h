#ifndef _ENTRY_H
#define _ENTRY_H
#include "SupportDefs.h"
#include "Path.h"
struct entry_ref {};
class BEntry {
public:
    BEntry(const char* path, bool traverse = false) {}
    BEntry(const BPath& path) {}
    status_t InitCheck() { return B_OK; }
    status_t Rename(const char* path, bool clobber = false) { return B_OK; }
    status_t Remove() { return B_OK; }
    BEntry() {}
    status_t SetTo(const char* path, bool traverse = false) { return B_OK; }
    status_t GetPath(BPath* path) const { return B_OK; }
    bool Exists() const { return false; }
    status_t GetRef(entry_ref* ref) const { return B_OK; }
    status_t GetName(char* buffer) { buffer[0] = 0; return B_OK; }
    bool IsDirectory() const { return false; }
    status_t GetSize(off_t* size) const { *size = 0; return B_OK; }
    void Unset() {}
};

inline status_t get_ref_for_path(const char* path, entry_ref* ref) { return B_OK; }
#endif
