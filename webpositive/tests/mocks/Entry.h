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
};
#endif
