#ifndef _MOCK_PATH_H
#define _MOCK_PATH_H
#include <string>
#include "SupportDefs.h"
#include "String.h"

class BPath {
public:
    BPath() {}
    BPath(const char* p) : path(p ? p : "") {}
    BPath(const BPath& other) : path(other.path) {}
    const char* Path() const { return path.c_str(); }
    const char* Leaf() const {
        size_t pos = path.find_last_of('/');
        if (pos != std::string::npos) return path.c_str() + pos + 1;
        return path.c_str();
    }
    status_t Append(const char* p) {
        if (!path.empty() && path.back() != '/') path += "/";
        path += p;
        return B_OK;
    }
    status_t Append(const BString& p) {
        return Append(p.String());
    }
    status_t GetParent(BPath* parent) const {
        size_t pos = path.find_last_of('/');
        if (pos != std::string::npos) parent->path = path.substr(0, pos);
        else parent->path = ".";
        return B_OK;
    }
private:
    std::string path;
};
#endif
