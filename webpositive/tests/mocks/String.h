#ifndef STRING_H
#define STRING_H
#include "SupportDefs.h"
#include <string>
#include <iostream>
#include <stdio.h>
#include <cstring>
#include <vector>

class BString {
public:
    std::string fString;
    // Buffer for LockBuffer
    std::vector<char> fBuffer;

    BString() {}
    BString(const char* str) : fString(str ? str : "") {}
    BString(const BString& other) : fString(other.fString) {}

    const char* String() const { return fString.c_str(); }
    int32 Length() const { return fString.length(); }

    int32 FindFirst(const char* str) const {
        size_t pos = fString.find(str);
        return (pos == std::string::npos) ? B_ERROR : (int32)pos;
    }

    int32 FindFirst(const char* str, int32 fromOffset) const {
        if (fromOffset < 0) fromOffset = 0;
        size_t pos = fString.find(str, fromOffset);
        return (pos == std::string::npos) ? B_ERROR : (int32)pos;
    }

    int32 FindFirst(char c) const {
         size_t pos = fString.find(c);
         return (pos == std::string::npos) ? B_ERROR : (int32)pos;
    }

    int32 FindFirst(char c, int32 fromOffset) const {
         if (fromOffset < 0) fromOffset = 0;
         size_t pos = fString.find(c, fromOffset);
         return (pos == std::string::npos) ? B_ERROR : (int32)pos;
    }

    bool EndsWith(const BString& str) const {
        if (str.Length() > Length()) return false;
        return fString.compare(Length() - str.Length(), str.Length(), str.fString) == 0;
    }

    int32 IFindFirst(const char* str) const {
        return FindFirst(str);
    }

    void CopyInto(BString& dest, int32 from, int32 length) const {
        if (from < 0) from = 0;
        if (length < 0) length = 0;
        if (from + length > fString.length()) length = fString.length() - from;
        dest.fString = fString.substr(from, length);
    }

    void CopyCharsInto(BString& dest, int32 from, int32 length) const {
        CopyInto(dest, from, length);
    }

    void ReplaceAll(const char* oldStr, const char* newStr) {
        size_t pos = 0;
        std::string o = oldStr;
        std::string n = newStr;
        while((pos = fString.find(o, pos)) != std::string::npos) {
            fString.replace(pos, o.length(), n);
            pos += n.length();
        }
    }

    void ReplaceAll(const char* oldStr, const BString& newStr) {
        ReplaceAll(oldStr, newStr.String());
    }

    void Remove(int32 from, int32 length) {
        if (from >= fString.length()) return;
        fString.erase(from, length);
    }

    BString& Append(const char* str, int32 length) {
        if (str && length > 0) fString.append(str, length);
        return *this;
    }

    BString& Append(const char* str) {
        if (str) fString.append(str);
        return *this;
    }

    bool operator==(const char* str) const { return fString == (str ? str : ""); }
    bool operator==(const BString& other) const { return fString == other.fString; }

    bool operator!=(const char* str) const { return !(*this == str); }

    bool operator<(const BString& other) const {
        return fString < other.fString;
    }

    char operator[](int32 index) const { return fString[index]; }

    void SetByteAt(int32 index, char c) { fString[index] = c; }

    char ByteAt(int32 index) const { return fString[index]; }

    void Insert(const char* str, int32 pos) { fString.insert(pos, str); }

    int Compare(const char* str, int32 len) const { return fString.compare(0, len, str, len); }

    BString& operator<<(const char* str) { fString += str; return *this; }
    BString& operator<<(const BString& str) { fString += str.fString; return *this; }
    BString& operator+=(const char* str) { fString += str; return *this; }
    BString& operator+=(const BString& str) { fString += str.fString; return *this; }

    BString& operator<<(int32 val) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", (int)val);
        fString += buf;
        return *this;
    }

    BString& operator<<(uint32 val) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%u", (unsigned int)val);
        fString += buf;
        return *this;
    }

    // Added for time_t (long)
    BString& operator<<(long val) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%ld", val);
        fString += buf;
        return *this;
    }

    BString& operator<<(unsigned long val) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%lu", val);
        fString += buf;
        return *this;
    }

    BString& operator<<(long long val) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%lld", val);
        fString += buf;
        return *this;
    }

    void SetTo(const char* str) { fString = str; }

    void Truncate(int32 newLength) {
        if (newLength < fString.length()) fString.resize(newLength);
    }

    bool StartsWith(const char* prefix) const {
        return fString.find(prefix) == 0;
    }

    void ToUpper() {
        for (size_t i = 0; i < fString.length(); ++i) {
            fString[i] = toupper(fString[i]);
        }
    }

    bool IsEmpty() const { return fString.empty(); }

    int ICompare(const char* str) const {
        return fString.compare(str);
    }

    char* LockBuffer(int32 size) {
        fBuffer.assign(size + 1, 0);
        if (fString.length() < size) {
             fString.resize(size, 0);
        }
        if (fString.length() > size) fBuffer.assign(fString.begin(), fString.begin() + size);
        else {
             fBuffer.assign(fString.begin(), fString.end());
             fBuffer.resize(size + 1, 0);
        }
        return &fBuffer[0];
    }

    BString& UnlockBuffer(int32 length = -1) {
        if (length < 0) length = strlen(&fBuffer[0]);
        fString.assign(&fBuffer[0], length);
        return *this;
    }
};

inline bool operator==(const char* str, const BString& bstr) {
    return bstr == str;
}
#endif
