#ifndef FILE_H
#define FILE_H

#include "SupportDefs.h"
#include "Entry.h"

#ifndef B_NO_INIT
#define B_NO_INIT 'noit'
#endif

// Forward declare BPath to break circular dependency with Entry.h
class BPath;

// File open modes from StorageDefs.h
#define B_READ_ONLY			0x0001
#define B_WRITE_ONLY		0x0002
#define B_READ_WRITE		0x0003
#define B_CREATE_FILE		0x0008
#define B_ERASE_FILE		0x0010


class BFile {
public:
    BFile() : fStatus(B_NO_INIT) {}
    BFile(const char* path, uint32 openMode) : fStatus(B_OK) {}
	BFile(const BPath& path, uint32 openMode);
    virtual ~BFile() {}

    status_t InitCheck() const { return fStatus; }

	status_t SetTo(const char* path, uint32 openMode) {
		fStatus = B_OK;
		return fStatus;
	}

    // Mocked methods that don't need to do anything for this test
    ssize_t Read(void* buffer, size_t size) { return 0; }
    ssize_t Write(const void* buffer, size_t size) { return size; }
    status_t GetSize(off_t* size) {
		if (size) *size = 0;
		return B_OK;
	}

private:
    status_t fStatus;
};

#endif // FILE_H
