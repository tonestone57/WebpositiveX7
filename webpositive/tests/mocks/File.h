#ifndef _MOCK_FILE_H
#define _MOCK_FILE_H
#include "SupportDefs.h"
#include <string>

enum {
    B_READ_ONLY = 1,
    B_WRITE_ONLY = 2,
    B_CREATE_FILE = 4,
    B_ERASE_FILE = 8
};

class BFile {
public:
    BFile() {}
    BFile(const char* path, uint32 mode) {}
    status_t InitCheck() { return B_OK; }
    ssize_t Write(const void* buffer, size_t size) {
        content.append((const char*)buffer, size);
        return size;
    }
    ssize_t Read(void* buffer, size_t size) { return 0; }
    status_t GetSize(off_t* size) { *size = 0; return B_OK; }
    status_t SetTo(const char* path, uint32 mode) { return B_OK; }
    void Unset() {}

    static std::string content;
};
#endif
