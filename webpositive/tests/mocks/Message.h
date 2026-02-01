#ifndef _MOCK_MESSAGE_H
#define _MOCK_MESSAGE_H
#include "SupportDefs.h"
#include "String.h"
#include "Entry.h"
#include <map>
#include <string>

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
    status_t FindString(const char* name, const char** value) const {
        if (strings.count(name)) {
            *value = strings.at(name).c_str();
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

    status_t FindData(const char* name, type_code type, const void** data, ssize_t* numBytes) const {
        return B_ERROR;
    }
    status_t FindRef(const char* name, entry_ref* ref) const {
        return B_OK;
    }
    status_t FindRef(const char* name, int32 index, entry_ref* ref) const {
        return B_OK;
    }

    status_t FindMessage(const char* name, BMessage* message) const {
        if (messages.count(name) && !messages.at(name).empty()) {
            *message = messages.at(name)[0];
            return B_OK;
        }
        return B_ERROR;
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

    status_t Flatten(BFile* file) const {
        file->Write(&what, sizeof(uint32));

        uint32 count = strings.size();
        file->Write(&count, sizeof(uint32));
        for (std::map<std::string, std::string>::const_iterator it = strings.begin(); it != strings.end(); ++it) {
            uint32 len = it->first.length();
            file->Write(&len, sizeof(uint32));
            file->Write(it->first.c_str(), len);
            len = it->second.length();
            file->Write(&len, sizeof(uint32));
            file->Write(it->second.c_str(), len);
        }

        count = int64s.size();
        file->Write(&count, sizeof(uint32));
        for (std::map<std::string, int64>::const_iterator it = int64s.begin(); it != int64s.end(); ++it) {
            uint32 len = it->first.length();
            file->Write(&len, sizeof(uint32));
            file->Write(it->first.c_str(), len);
            file->Write(&it->second, sizeof(int64));
        }

        count = int32s.size();
        file->Write(&count, sizeof(uint32));
        for (std::map<std::string, int32>::const_iterator it = int32s.begin(); it != int32s.end(); ++it) {
            uint32 len = it->first.length();
            file->Write(&len, sizeof(uint32));
            file->Write(it->first.c_str(), len);
            file->Write(&it->second, sizeof(int32));
        }

        count = uint32s.size();
        file->Write(&count, sizeof(uint32));
        for (std::map<std::string, uint32>::const_iterator it = uint32s.begin(); it != uint32s.end(); ++it) {
            uint32 len = it->first.length();
            file->Write(&len, sizeof(uint32));
            file->Write(it->first.c_str(), len);
            file->Write(&it->second, sizeof(uint32));
        }

        count = messages.size();
        file->Write(&count, sizeof(uint32));
        for (std::map<std::string, std::vector<BMessage> >::const_iterator it = messages.begin(); it != messages.end(); ++it) {
            uint32 len = it->first.length();
            file->Write(&len, sizeof(uint32));
            file->Write(it->first.c_str(), len);

            uint32 vecSize = it->second.size();
            file->Write(&vecSize, sizeof(uint32));
            for (size_t i = 0; i < vecSize; i++) {
                it->second[i].Flatten(file);
            }
        }
        return B_OK;
    }

    status_t Unflatten(BFile* file) {
        if (file->Read(&what, sizeof(uint32)) != sizeof(uint32)) return B_ERROR;

        uint32 count;
        if (file->Read(&count, sizeof(uint32)) != sizeof(uint32)) return B_ERROR;
        if (count > 10000) return B_ERROR; // Sanity check for mock
        for (uint32 i=0; i<count; i++) {
            uint32 len;
            file->Read(&len, sizeof(uint32));
            std::string key(len, '\0');
            file->Read(&key[0], len);

            file->Read(&len, sizeof(uint32));
            std::string val(len, '\0');
            file->Read(&val[0], len);

            strings[key] = val;
        }

        if (file->Read(&count, sizeof(uint32)) != sizeof(uint32)) return B_ERROR;
        for (uint32 i=0; i<count; i++) {
            uint32 len;
            file->Read(&len, sizeof(uint32));
            std::string key(len, '\0');
            file->Read(&key[0], len);

            int64 val;
            file->Read(&val, sizeof(int64));

            int64s[key] = val;
        }

        if (file->Read(&count, sizeof(uint32)) != sizeof(uint32)) return B_ERROR;
        for (uint32 i=0; i<count; i++) {
            uint32 len;
            file->Read(&len, sizeof(uint32));
            std::string key(len, '\0');
            file->Read(&key[0], len);

            int32 val;
            file->Read(&val, sizeof(int32));

            int32s[key] = val;
        }

        if (file->Read(&count, sizeof(uint32)) != sizeof(uint32)) return B_ERROR;
        for (uint32 i=0; i<count; i++) {
            uint32 len;
            file->Read(&len, sizeof(uint32));
            std::string key(len, '\0');
            file->Read(&key[0], len);

            uint32 val;
            file->Read(&val, sizeof(uint32));

            uint32s[key] = val;
        }

        if (file->Read(&count, sizeof(uint32)) != sizeof(uint32)) return B_ERROR;
        for (uint32 i=0; i<count; i++) {
            uint32 len;
            file->Read(&len, sizeof(uint32));
            std::string key(len, '\0');
            file->Read(&key[0], len);

            uint32 vecSize;
            file->Read(&vecSize, sizeof(uint32));
            for (uint32 j = 0; j < vecSize; j++) {
                BMessage msg;
                msg.Unflatten(file);
                messages[key].push_back(msg);
            }
        }
        return B_OK;
    }

    void MakeEmpty() {
        int64s.clear();
        int32s.clear();
        uint32s.clear();
        strings.clear();
        messages.clear();
    }

    status_t GetInfo(const char* name, type_code* type, int32* count) const {
        if (messages.count(name)) *count = (int32)messages.at(name).size();
        else *count = 0;
        return B_OK;
    }

    uint32 what;
    std::map<std::string, int64> int64s;
    std::map<std::string, int32> int32s;
    std::map<std::string, uint32> uint32s;
    std::map<std::string, std::string> strings;
    std::map<std::string, std::vector<BMessage> > messages;
};
#endif
