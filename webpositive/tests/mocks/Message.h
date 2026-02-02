#ifndef _MOCK_MESSAGE_H
#define _MOCK_MESSAGE_H
#include "SupportDefs.h"
#include "String.h"
#include "Entry.h"
#include <map>
#include <string>
#include <vector>

class BFile;

const uint32 B_REFS_RECEIVED = 'RRCV';

class BMessage {
public:
    BMessage(uint32 what = 0) : what(what) {}

    status_t FindInt64(const char* name, int64* value) const {
        return FindInt64(name, 0, value);
    }
    status_t FindInt64(const char* name, int32 index, int64* value) const {
        if (int64s.count(name) && index >= 0 && index < (int32)int64s.at(name).size()) {
            *value = int64s.at(name)[index];
            return B_OK;
        }
        return B_ERROR;
    }
    status_t AddInt64(const char* name, int64 value) {
        int64s[name].push_back(value);
        return B_OK;
    }

    status_t FindString(const char* name, const char** value) const {
        return FindString(name, 0, value);
    }
    status_t FindString(const char* name, int32 index, const char** value) const {
        if (strings.count(name) && index >= 0 && index < (int32)strings.at(name).size()) {
            *value = strings.at(name)[index].c_str();
            return B_OK;
        }
        return B_ERROR;
    }

    status_t FindString(const char* name, BString* value) const {
        return FindString(name, 0, value);
    }
    status_t FindString(const char* name, int32 index, BString* value) const {
        if (strings.count(name) && index >= 0 && index < (int32)strings.at(name).size()) {
            *value = strings.at(name)[index].c_str();
            return B_OK;
        }
        return B_ERROR;
    }
    status_t AddString(const char* name, const char* value) {
        strings[name].push_back(value);
        return B_OK;
    }
    status_t AddString(const char* name, const BString& value) {
        return AddString(name, value.String());
    }

    status_t FindInt32(const char* name, int32* value) const {
        return FindInt32(name, 0, value);
    }
    status_t FindInt32(const char* name, int32 index, int32* value) const {
        if (int32s.count(name) && index >= 0 && index < (int32)int32s.at(name).size()) {
            *value = int32s.at(name)[index];
            return B_OK;
        }
        return B_ERROR;
    }
    status_t AddInt32(const char* name, int32 value) {
        int32s[name].push_back(value);
        return B_OK;
    }

    status_t FindUInt32(const char* name, uint32* value) const {
        return FindUInt32(name, 0, value);
    }
    status_t FindUInt32(const char* name, int32 index, uint32* value) const {
        if (uint32s.count(name) && index >= 0 && index < (int32)uint32s.at(name).size()) {
            *value = uint32s.at(name)[index];
            return B_OK;
        }
        return B_ERROR;
    }
    status_t AddUInt32(const char* name, uint32 value) {
        uint32s[name].push_back(value);
        return B_OK;
    }

    status_t FindData(const char* name, type_code type, const void** data, ssize_t* numBytes) const {
        return FindData(name, type, 0, data, numBytes);
    }
    status_t FindData(const char* name, type_code type, int32 index, const void** data, ssize_t* numBytes) const {
        if (dataItems.count(name) && index >= 0 && index < (int32)dataItems.at(name).size()) {
             *data = dataItems.at(name)[index].data();
             *numBytes = dataItems.at(name)[index].size();
             return B_OK;
        }
        // Fallback to strings if needed (simulating BMessage behavior where strings are data too)
        if (strings.count(name) && index >= 0 && index < (int32)strings.at(name).size()) {
            *data = strings.at(name)[index].c_str();
            *numBytes = strings.at(name)[index].length() + 1; // Include null terminator
            return B_OK;
        }
        return B_ERROR;
    }
    status_t AddData(const char* name, type_code type, const void* data, ssize_t numBytes) {
        std::vector<uint8_t> vec;
        vec.assign((const uint8_t*)data, (const uint8_t*)data + numBytes);
        dataItems[name].push_back(vec);
        return B_OK;
    }

    status_t FindRef(const char* name, entry_ref* ref) const {
        return B_OK;
    }
    status_t FindRef(const char* name, int32 index, entry_ref* ref) const {
        return B_OK;
    }
    status_t AddRef(const char* name, const entry_ref* ref) {
        return B_OK;
    }

    status_t FindMessage(const char* name, BMessage* message) const {
        return FindMessage(name, 0, message);
    }
    status_t FindMessage(const char* name, int32 index, BMessage* message) const {
        if (messages.count(name) && index >= 0 && index < (int32)messages.at(name).size()) {
            *message = messages.at(name)[index];
            return B_OK;
        }
        return B_ERROR;
    }
    status_t AddMessage(const char* name, const BMessage* message) {
        messages[name].push_back(*message);
        return B_OK;
    }

    status_t Flatten(BFile* file) { return B_OK; }
    status_t Unflatten(BFile* file) { return B_OK; }

    void MakeEmpty() {
        int64s.clear();
        int32s.clear();
        uint32s.clear();
        strings.clear();
        messages.clear();
    }

    status_t GetInfo(const char* name, type_code* type, int32* count) const {
        if (messages.count(name)) {
            *count = messages.at(name).size();
            return B_OK;
        }
        if (strings.count(name)) {
            *count = strings.at(name).size();
            return B_OK;
        }
        if (int64s.count(name)) {
            *count = int64s.at(name).size();
            return B_OK;
        }
        if (int32s.count(name)) {
            *count = int32s.at(name).size();
            return B_OK;
        }
        if (uint32s.count(name)) {
            *count = uint32s.at(name).size();
            return B_OK;
        }
        if (dataItems.count(name)) {
            *count = dataItems.at(name).size();
            return B_OK;
        }
        *count = 0;
        return B_ENTRY_NOT_FOUND;
    }

    uint32 what;
    std::map<std::string, std::vector<int64> > int64s;
    std::map<std::string, std::vector<int32> > int32s;
    std::map<std::string, std::vector<uint32> > uint32s;
    std::map<std::string, std::vector<std::string> > strings;
    std::map<std::string, std::vector<BMessage> > messages;
    std::map<std::string, std::vector<std::vector<uint8_t> > > dataItems;
};
#endif
