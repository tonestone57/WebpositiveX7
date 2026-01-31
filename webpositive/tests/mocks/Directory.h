#ifndef _MOCK_DIRECTORY_H
#define _MOCK_DIRECTORY_H
#include "SupportDefs.h"
#include "Path.h"
#include <sys/types.h>

class BEntry;

class BDirectory {
public:
    BDirectory(const BEntry* entry) {}
    BDirectory(const char* path) {}
    status_t InitCheck() { return B_OK; }
    void Rewind() {}
    status_t GetNextEntry(BEntry* entry, bool traverse = false) { return B_ENTRY_NOT_FOUND; }
};

status_t create_directory(const char* path, mode_t mode);
#endif
