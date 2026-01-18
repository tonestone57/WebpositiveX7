#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <SupportDefs.h>

// Mock create_directory global function
inline status_t create_directory(const char* path, mode_t mode) {
    // In our mock file system, directories are just paths.
    // We don't explicitly store directory nodes in the flat map,
    // but we can pretend it succeeded.
    return B_OK;
}

class BDirectory {
public:
	BDirectory(const char* path = NULL) {}
};

#endif
