#ifndef _MOCK_DIRECTORY_H
#define _MOCK_DIRECTORY_H
#include "SupportDefs.h"
#include "Path.h"
#include <sys/types.h>

class BEntry;

class BDirectory {
public:
    BDirectory(const BEntry* entry) {}
    status_t InitCheck() { return B_OK; }
};

status_t create_directory(const char* path, mode_t mode);
#endif
