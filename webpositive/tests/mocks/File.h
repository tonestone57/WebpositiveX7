#ifndef _MOCK_FILE_H
#define _MOCK_FILE_H
#include "SupportDefs.h"
#include "Entry.h"
#include "String.h"
#include "Node.h"
#include <string>

enum {
    B_READ_ONLY = 1,
    B_WRITE_ONLY = 2,
    B_CREATE_FILE = 4,
    B_ERASE_FILE = 8
};

class BFile : public BNode {
public:
    BFile() {}
    BFile(const char* path, uint32 mode) {}
    BFile(const BEntry* entry, uint32 mode) {}
    status_t InitCheck() { return B_OK; }
    ssize_t Write(const void* buffer, size_t size) {
        content.append((const char*)buffer, size);
        return size;
    }
    ssize_t Read(void* buffer, size_t size) {
        if (size > content.length()) size = content.length();
        memcpy(buffer, content.data(), size);
        return size;
    }
    status_t GetSize(off_t* size) { *size = content.length(); return B_OK; }
    status_t SetTo(const char* path, uint32 mode) { return B_OK; }
    status_t SetTo(const BEntry* entry, uint32 mode) { return B_OK; }
    void Unset() {}

    ssize_t ReadAttrString(const char* name, BString* result) { return B_OK; }
    ssize_t WriteAttrString(const char* name, const BString* data) { return B_OK; }

    static std::string content;
};
#endif
