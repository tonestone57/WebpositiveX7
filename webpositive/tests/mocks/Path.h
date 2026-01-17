#ifndef PATH_H
#define PATH_H

#include <SupportDefs.h>
#include <String.h>

class BPath {
public:
	BPath() {}
	status_t Append(const char* path) { return B_OK; }
	const char* Path() const { return ""; }
};

#endif
