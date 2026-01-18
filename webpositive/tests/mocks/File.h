/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FILE_H
#define _FILE_H

#include "SupportDefs.h"
#include "Entry.h"

class BFile {
public:
	BFile() {}
	BFile(const char* path, uint32 openMode) {}
	BFile(const BEntry* entry, uint32 openMode) {}

	status_t SetTo(const char* path, uint32 openMode) { return B_OK; }
	status_t InitCheck() { return B_OK; }
	ssize_t Read(void* buffer, size_t size) { return size; }
	ssize_t Write(const void* buffer, size_t size) { return size; }
};

#endif // _FILE_H
