#ifndef _MOCK_PATH_H
#define _MOCK_PATH_H
#include "SupportDefs.h"
#include "EntryRef.h"
#include <string>

class BPath {
public:
    std::string path;

    BPath() {}
    BPath(const char* p) { if (p) path = p; }
    BPath(const entry_ref* ref) { if (ref) path = ref->path; }

    status_t SetTo(const char* p) { path = p ? p : ""; return B_OK; }
    status_t SetTo(const entry_ref* ref) {
        if (ref) path = ref->path;
        else path = "";
        return B_OK;
    }
    status_t Append(const char* component) {
        if (!path.empty() && path.back() != '/') path += "/";
        path += component;
        return B_OK;
    }
    const char* Path() const { return path.c_str(); }
    const char* Leaf() const {
        size_t pos = path.rfind('/');
        if (pos != std::string::npos) return path.c_str() + pos + 1;
        return path.c_str();
    }
    status_t GetParent(BPath* parent) const {
        size_t pos = path.rfind('/');
        if (pos != std::string::npos) {
             parent->SetTo(path.substr(0, pos).c_str());
        } else {
             parent->SetTo(".");
        }
        return B_OK;
    }
};
#endif
