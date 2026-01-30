#ifndef _MOCK_MESSAGE_H
#define _MOCK_MESSAGE_H
#include "SupportDefs.h"
#include "String.h"
#include <map>
#include <string>

typedef int type_code;

class BFile;

class BMessage {
public:
    BMessage(uint32 what = 0) : what(what) {}

    status_t FindInt64(const char* name, int64* value) const {
        if (int64s.count(name)) {
            *value = int64s.at(name);
            return B_OK;
        }
        return B_ERROR;
    }
    status_t AddInt64(const char* name, int64 value) {
        int64s[name] = value;
        return B_OK;
    }

    status_t FindInt32(const char* name, int32* value) const {
        if (int32s.count(name)) {
            *value = int32s.at(name);
            return B_OK;
        }
        return B_ERROR;
    }
    status_t AddInt32(const char* name, int32 value) {
        int32s[name] = value;
        return B_OK;
    }

    status_t FindUInt32(const char* name, uint32* value) const {
        if (uint32s.count(name)) {
            *value = uint32s.at(name);
            return B_OK;
        }
        return B_ERROR;
    }
    status_t AddUInt32(const char* name, uint32 value) {
        uint32s[name] = value;
        return B_OK;
    }

    status_t FindString(const char* name, BString* value) const {
        if (strings.count(name)) {
            *value = strings.at(name).c_str();
            return B_OK;
        }
        return B_ERROR;
    }
    status_t AddString(const char* name, const char* value) {
        strings[name] = value;
        return B_OK;
    }
    status_t AddString(const char* name, const BString& value) {
        return AddString(name, value.String());
    }

    status_t FindMessage(const char* name, BMessage* message) const {
        if (messages.count(name)) {
            *message = messages.at(name);
            return B_OK;
        }
        return B_ERROR;
    }
    status_t FindMessage(const char* name, int32 index, BMessage* message) const {
        // Index not supported in simple map mock, but BrowsingHistory uses index loop
        // for "history item" 0..N.
        // I need to support lists or multiple items with same name?
        // BrowsingHistory.cpp: settingsArchive.FindMessage("history item", i, &historyItemArchive)
        // I need to support lists.
        // But for "date time", it is single.
        // Let's assume single for now for "date time".
        // But "history item" needs list.
        // Wait, BrowsingHistoryItemTest doesn't test loading full history list, only single item.
        // So single map entry is enough for "date time".
        if (index == 0 && messages.count(name)) {
            *message = messages.at(name);
            return B_OK;
        }
        return B_ERROR;
    }
    status_t AddMessage(const char* name, const BMessage* message) {
        messages[name] = *message;
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
        // Mock implementation for iteration
        if (messages.count(name)) *count = 1;
        else *count = 0;
        return B_OK;
    }

    uint32 what;
    std::map<std::string, int64> int64s;
    std::map<std::string, int32> int32s;
    std::map<std::string, uint32> uint32s;
    std::map<std::string, std::string> strings;
    std::map<std::string, BMessage> messages;
};
#endif
