#ifndef PATH_H
#define PATH_H

#include <SupportDefs.h>
#include <String.h>

class BPath {
public:
	BPath() {}
    BPath(const char* path) : fPath(path) {}
    BPath(const BPath& other) : fPath(other.fPath) {}

	status_t Append(const char* path) {
        if (fPath.Length() > 0 && fPath[fPath.Length()-1] != '/')
            fPath << "/";
        fPath << path;
        return B_OK;
    }

	const char* Path() const { return fPath.String(); }

private:
    BString fPath;
};

#endif
