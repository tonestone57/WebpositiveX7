#ifndef FILE_H
#define FILE_H

#include <SupportDefs.h>

enum {
	B_READ_ONLY = 0,
	B_WRITE_ONLY = 1,
	B_CREATE_FILE = 2,
	B_ERASE_FILE = 4
};

class BFile {
public:
	BFile() {}
	BFile(const char* path, uint32 mode) {}
	status_t SetTo(const char* path, uint32 mode) { return B_OK; }
	status_t InitCheck() const { return B_OK; }
	ssize_t Write(const void* buffer, size_t size) { return size; }
};

#endif
