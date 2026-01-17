#ifndef MESSAGE_H
#define MESSAGE_H

#include <SupportDefs.h>
#include <String.h>

class BFile;

class BMessage {
public:
	uint32 what;
	BMessage(uint32 what = 0) : what(what) {}
	~BMessage() {}

	status_t AddInt64(const char* name, int64 val) { return B_OK; }
	status_t FindInt64(const char* name, int64* val) const { return B_OK; }
	status_t AddString(const char* name, const char* val) { return B_OK; }
	status_t FindString(const char* name, const char** val) const {
		// Mock implementation
		*val = "";
		return B_OK;
	}
	status_t FindString(const char* name, BString* val) const { return B_OK; }
	status_t AddMessage(const char* name, const BMessage* msg) { return B_OK; }
	status_t FindMessage(const char* name, BMessage* msg) const { return B_OK; }
	status_t FindMessage(const char* name, int32 index, BMessage* msg) const { return B_ERROR; } // Stop iteration
	status_t AddUInt32(const char* name, uint32 val) { return B_OK; }
	status_t FindUInt32(const char* name, uint32* val) const { return B_OK; }
	status_t AddInt32(const char* name, int32 val) { return B_OK; }
	status_t FindInt32(const char* name, int32* val) const { return B_OK; }

	status_t GetInfo(const char* name, type_code* typeFound, int32* countFound = NULL) const {
		if (countFound) *countFound = 0;
		return B_ERROR;
	}

	status_t Flatten(BFile* file) const { return B_OK; }
	status_t Unflatten(BFile* file) { return B_OK; }
	void MakeEmpty() {}
};

#endif
