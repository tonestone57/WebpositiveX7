#ifndef _FILE_H
#define _FILE_H

#include "SupportDefs.h"

enum {
    B_READ_ONLY = 0x01,
    B_WRITE_ONLY = 0x02,
    B_CREATE_FILE = 0x04,
    B_ERASE_FILE = 0x08
};

class BFile {
public:
    BFile() {}
    BFile(const char* path, uint32 openMode) {}
    status_t InitCheck() { return B_OK; }
    ssize_t Write(const void* buffer, size_t size) { return size; }
    status_t SetTo(const char* path, uint32 openMode) { return B_OK; }

    // For ImportHistory
    status_t GetSize(off_t* size) { *size = 0; return B_OK; }
    ssize_t Read(void* buffer, size_t size) { return 0; }
};

#endif
