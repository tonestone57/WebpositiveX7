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
    B_ERASE_FILE = 8,
    B_OPEN_AT_END = 16
};

class BFile : public BNode {
public:
    off_t fPosition;
    BFile() : BNode(), fPosition(0) {}
    BFile(const char* path, uint32 mode) : BNode(), fPosition(0) {}
    BFile(const BEntry* entry, uint32 mode) : BNode(entry), fPosition(0) {}
    status_t InitCheck() { return B_OK; }
    ssize_t Write(const void* buffer, size_t size) {
        content.append((const char*)buffer, size);
        fPosition += size;
        return size;
    }
    ssize_t Read(void* buffer, size_t size) {
        if (fPosition >= (off_t)content.length()) return 0;
        size_t available = content.length() - fPosition;
        if (size > available) size = available;
        memcpy(buffer, content.data() + fPosition, size);
        fPosition += size;
        return size;
    }
    off_t Position() const { return fPosition; }
    off_t Seek(off_t offset, int32 mode) {
        if (mode == SEEK_SET) fPosition = offset;
        else if (mode == SEEK_CUR) fPosition += offset;
        else if (mode == SEEK_END) fPosition = content.length() + offset;
        return fPosition;
    }
    status_t GetSize(off_t* size) { *size = content.length(); return B_OK; }
    status_t SetTo(const char* path, uint32 mode) {
        fPosition = 0;
        if (mode & B_ERASE_FILE) content = "";
        return B_OK;
    }
    status_t SetTo(const BEntry* entry, uint32 mode) {
        fPosition = 0;
        if (mode & B_ERASE_FILE) content = "";
        return B_OK;
    }
    void Unset() {}

    // Using BNode::ReadAttrString
    using BNode::ReadAttrString;

    ssize_t WriteAttrString(const char* name, const BString* data) {
         if (data) fAttributes[name] = data->String();
         return B_OK;
    }

    static std::string content;
};
#endif
