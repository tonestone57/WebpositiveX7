#ifndef _MOCK_STRING_H
#define _MOCK_STRING_H
#include <string>
#include <algorithm>
#include <cctype>
#include <vector>
#include <cstring>
#include "SupportDefs.h"

class BString {
public:
    std::string s;
    BString() {}
    BString(const char* str) : s(str ? str : "") {}
    BString(const BString& other) : s(other.s) {}

    const char* String() const { return s.c_str(); }
    int32 Length() const { return (int32)s.length(); }
    void Truncate(int32 len) { if (len < (int32)s.length()) s.resize(len); }
    void ReplaceAll(const char* a, const char* b) {
        if (!a || !*a) return;
        size_t pos = 0;
        std::string sa = a;
        std::string sb = b ? b : "";
        while((pos = s.find(sa, pos)) != std::string::npos) {
            s.replace(pos, sa.length(), sb);
            pos += sb.length();
        }
    }
    void ReplaceAll(const char* a, const BString& b) { ReplaceAll(a, b.String()); }

    int32 FindFirst(char c) const {
        size_t pos = s.find(c);
        return pos == std::string::npos ? B_ERROR : (int32)pos;
    }
    int32 FindFirst(const char* str) const {
        size_t pos = s.find(str);
        return pos == std::string::npos ? B_ERROR : (int32)pos;
    }

    // Case insensitive find
    int32 IFindFirst(const char* str) const {
        std::string upperS = s;
        std::string upperStr = str;
        std::transform(upperS.begin(), upperS.end(), upperS.begin(), ::toupper);
        std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);
        size_t pos = upperS.find(upperStr);
        return pos == std::string::npos ? B_ERROR : (int32)pos;
    }

    void Append(const char* str, int32 len) { s.append(str, len); }
    BString& operator+=(char c) { s += c; return *this; }
    BString& operator+=(const char* str) { s += str; return *this; }
    BString& operator+=(const BString& str) { s += str.s; return *this; }

    BString& operator<<(const char* str) { s += str; return *this; }
    BString& operator<<(const BString& str) { s += str.s; return *this; }
    BString& operator<<(int val) { s += std::to_string(val); return *this; }
    BString& operator<<(long val) { s += std::to_string(val); return *this; }
    BString& operator<<(uint32 val) { s += std::to_string(val); return *this; }

    char operator[](int32 index) const { return s[index]; }
    char& operator[](int32 index) { return s[index]; }

    char* LockBuffer(int32 size) {
        s.resize(size);
        return &s[0];
    }
    void UnlockBuffer(int32 size = -1) {
        if (size >= 0) s.resize(size);
        else s.resize(strlen(s.c_str()));
    }

    void CopyCharsInto(BString& dest, int32 fromOffset, int32 charCount) const {
        if (fromOffset < 0 || fromOffset >= (int32)s.length()) return;
        dest.s = s.substr(fromOffset, charCount);
    }
    void CopyInto(BString& dest, int32 fromOffset, int32 charCount) const {
        CopyCharsInto(dest, fromOffset, charCount);
    }

    void Remove(int32 fromOffset, int32 charCount) {
        if (fromOffset < 0 || fromOffset >= (int32)s.length()) return;
        s.erase(fromOffset, charCount);
    }

    int Compare(const char* str, int32 n) const {
        return strncmp(s.c_str(), str, n);
    }
    int Compare(const BString& str) const {
        return s.compare(str.s);
    }

    friend bool operator<(const BString& a, const BString& b) { return a.s < b.s; }
    friend bool operator==(const BString& a, const BString& b) { return a.s == b.s; }
    friend bool operator!=(const BString& a, const BString& b) { return a.s != b.s; }
    friend bool operator==(const BString& a, const char* b) { return a.s == (b ? b : ""); }
    friend bool operator==(const char* a, const BString& b) { return b == a; }
    friend bool operator!=(const BString& a, const char* b) { return !(a == b); }
};
#endif
