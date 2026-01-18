#ifndef _MOCK_B_STRING_H
#define _MOCK_B_STRING_H

#include <string.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdint.h>
#include "SupportDefs.h"

class BString {
public:
    BString() : fString("") {}
    BString(const char* str) : fString(str ? str : "") {}
    BString(const BString& other) : fString(other.fString) {}

    const char* String() const { return fString.c_str(); }
    int32 Length() const { return (int32)fString.length(); }

    bool operator==(const BString& other) const { return fString == other.fString; }
    bool operator==(const char* other) const { return fString == (other ? other : ""); }
    bool operator!=(const BString& other) const { return fString != other.fString; }
    bool operator<(const BString& other) const { return fString < other.fString; }

    BString& operator=(const char* str) { fString = str ? str : ""; return *this; }

    int32 FindFirst(const char* str) const {
        size_t pos = fString.find(str);
        return pos == std::string::npos ? -1 : (int32)pos;
    }

    int32 FindFirst(char c, int32 fromIndex = 0) const {
        size_t pos = fString.find(c, fromIndex);
        return pos == std::string::npos ? -1 : (int32)pos;
    }

    void ReplaceAll(const char* from, const char* to) {
        size_t start_pos = 0;
        std::string s_from = from;
        std::string s_to = to;
        while((start_pos = fString.find(s_from, start_pos)) != std::string::npos) {
            fString.replace(start_pos, s_from.length(), s_to);
            start_pos += s_to.length();
        }
    }

    void ReplaceAll(const char* from, const BString& to) {
        ReplaceAll(from, to.String());
    }

    BString& operator<<(const char* str) {
        fString += str;
        return *this;
    }

    BString& operator<<(const BString& str) {
        fString += str.fString;
        return *this;
    }

    BString& operator<<(int value) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d", value);
        fString += buffer;
        return *this;
    }

    BString& operator+=(const char* str) {
        fString += str;
        return *this;
    }

    BString& operator+=(const BString& str) {
        fString += str.fString;
        return *this;
    }

    char ByteAt(int32 index) const {
        if (index < 0 || index >= Length()) return 0;
        return fString[index];
    }

    char operator[](int index) const {
        return ByteAt(index);
    }

    char* LockBuffer(int32 size) {
        fString.resize(size);
        return &fString[0];
    }

    void UnlockBuffer(int32 length) {
        // Assume length is correct
    }

    void CopyInto(BString& dest, int32 from, int32 length) const {
        if (from < 0) from = 0;
        if (from + length > Length()) length = Length() - from;
        if (length < 0) length = 0;
        dest.fString = fString.substr(from, length);
    }

    void CopyCharsInto(BString& dest, int32 from, int32 length) const {
        CopyInto(dest, from, length);
    }

    void CopyCharsInto(char* dest, int32 from, int32 length) const {
        if (from < 0) from = 0;
        if (from + length > Length()) length = Length() - from;
        if (length < 0) length = 0;
        std::string sub = fString.substr(from, length);
        memcpy(dest, sub.c_str(), length);
        dest[length] = '\0';
    }

    void Append(const char* str, int32 length) {
        fString.append(str, length);
    }

    bool IsEmpty() const { return fString.empty(); }

    // Missing methods for URLHandler
    void SetByteAt(int32 index, char c) {
        if (index >= 0 && index < Length()) {
            fString[index] = c;
        }
    }

    void Insert(const char* str, int32 index) {
        if (index < 0) index = 0;
        if (index > Length()) index = Length();
        fString.insert(index, str);
    }

    void Remove(int32 index, int32 length) {
         if (index < 0) index = 0;
         if (index >= Length()) return;
         fString.erase(index, length);
    }

    int Compare(const char* str, int32 len) const {
        return fString.compare(0, len, str);
    }

    friend BString operator+(const BString& a, const BString& b) {
        BString res(a);
        res += b;
        return res;
    }

private:
    std::string fString;
};

// Global operators
inline bool operator==(const char* a, const BString& b) {
    return b == a;
}

#endif
