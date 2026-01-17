#ifndef STRING_H
#define STRING_H
#include "SupportDefs.h"
#include <string>
#include <iostream>
#include <stdio.h>

class BString {
public:
    std::string fString;
    BString() {}
    BString(const char* str) : fString(str ? str : "") {}
    BString(const BString& other) : fString(other.fString) {}

    const char* String() const { return fString.c_str(); }
    int32 Length() const { return fString.length(); }

    int32 FindFirst(const char* str) const {
        size_t pos = fString.find(str);
        return (pos == std::string::npos) ? B_ERROR : (int32)pos;
    }
    int32 FindFirst(char c) const {
         size_t pos = fString.find(c);
         return (pos == std::string::npos) ? B_ERROR : (int32)pos;
    }

    // Case insensitive find
    int32 IFindFirst(const char* str) const {
        // Just forward to normal find for mock
        return FindFirst(str);
    }

    void CopyInto(BString& dest, int32 from, int32 length) const {
        dest.fString = fString.substr(from, length);
    }

    void CopyCharsInto(BString& dest, int32 from, int32 length) const {
        dest.fString = fString.substr(from, length);
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

    bool operator==(const char* str) const { return fString == (str ? str : ""); }
    bool operator==(const BString& other) const { return fString == other.fString; }

    // Needed for std::map key
    bool operator<(const BString& other) const {
        return fString < other.fString;
    }

    char operator[](int32 index) const { return fString[index]; }

    void SetByteAt(int32 index, char c) { fString[index] = c; }

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

    void SetTo(const char* str) { fString = str; }

    // Helper for Truncate
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

    // ICompare (Case insensitive)
    int ICompare(const char* str) const {
        // Just do simple compare for mock
        return fString.compare(str);
    }
};

// Global operators for comparisons
inline bool operator==(const char* str, const BString& bstr) {
    return bstr == str;
}
#endif
