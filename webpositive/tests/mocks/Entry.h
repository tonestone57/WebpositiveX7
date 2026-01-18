#ifndef ENTRY_H
#define ENTRY_H

#include <SupportDefs.h>
#include <string>

class BEntry {
public:
	BEntry(const char* path = NULL) {
        if (path) fPath = path;
    }

    status_t SetTo(const char* path, bool traverse = false) {
        if (path) fPath = path;
        else fPath = "";
        return B_OK;
    }

    const char* Path() const { return fPath.c_str(); }

    // For mocks
    std::string fPath;
};

#endif
