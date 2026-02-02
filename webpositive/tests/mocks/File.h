#ifndef _MOCK_FILE_H
#define _MOCK_FILE_H
#include "SupportDefs.h"
#include "Entry.h"
#include "String.h"
#include "Node.h"
#include "MockFileSystem.h"
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

    off_t Seek(off_t offset, uint32 seekMode) {
        if (seekMode == SEEK_SET) fPosition = offset;
        else if (seekMode == SEEK_CUR) fPosition += offset;
        else if (seekMode == SEEK_END) fPosition = content.length() + offset;
        return fPosition;
    }

    BFile(const char* path, uint32 mode) : BNode(), fPosition(0) {
        MockFileSystem::sOpenCount++;
        // Load attrs if exists
        MockEntryData data;
        if (MockFileSystem::GetEntry(path, &data)) {
            fAttributes = data.attributes;
        }
        if (mode & B_OPEN_AT_END) fPosition = content.length();
    }

    BFile(const BEntry* entry, uint32 mode) : BNode(entry), fPosition(0) {
        MockFileSystem::sOpenCount++;
        if (mode & B_OPEN_AT_END) fPosition = content.length();
    }

    BFile(const entry_ref* ref, uint32 mode) : BNode(), fPosition(0) {
        MockFileSystem::sOpenCount++;
        if (ref) {
             MockEntryData data;
             if (MockFileSystem::GetEntry(ref->path, &data)) {
                 fAttributes = data.attributes;
             }
        }
        if (mode & B_OPEN_AT_END) fPosition = content.length();
    }

    status_t InitCheck() { return B_OK; }

    ssize_t Write(const void* buffer, size_t size) {
        // Simple append behavior for Write as commonly used in these tests
        // If we wanted to be strictly correct with fPosition:
        // if (fPosition < content.length()) { replace... } else { append... }
        // But for now, sticking to append to avoid breaking tests relying on it
        // unless they explicitly Seek.
        content.append((const char*)buffer, size);
        fPosition = content.length();
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

    status_t GetSize(off_t* size) { *size = content.length(); return B_OK; }
    status_t SetTo(const char* path, uint32 mode) { fPosition = 0; return B_OK; }
    status_t SetTo(const BEntry* entry, uint32 mode) { fPosition = 0; return B_OK; }
    void Unset() {}

    // Using BNode::ReadAttrString
    using BNode::ReadAttrString;

    ssize_t WriteAttrString(const char* name, const BString* data) {
         if (data) fAttributes[name] = data->String();
         return B_OK;
    }

    ssize_t WriteAttr(const char* name, type_code type, off_t offset, const void* buffer, size_t size) {
        return size;
    }

    static std::string content;
};
#endif
