/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FILE_H
#define _FILE_H

#include "SupportDefs.h"
#include "Entry.h"
#include <map>
#include <string>
#include <stdio.h>
#include <cstring>

class BFile {
public:
	// Static map to simulate file system: path -> content
	static std::map<std::string, std::string> sFileSystem;

	BFile() : fInitStatus(B_NO_INIT) {}

	BFile(const char* path, uint32 openMode) {
		SetTo(path, openMode);
	}

	BFile(const BEntry* entry, uint32 openMode) {
        if (entry)
             SetTo(entry->Path(), openMode);
        else
             fInitStatus = B_BAD_VALUE;
	}

	status_t SetTo(const char* path, uint32 openMode) {
		fPath = path ? path : "";
		fOpenMode = openMode;

        if (fPath.length() == 0) {
            fInitStatus = B_BAD_VALUE;
            return fInitStatus;
        }

		if (openMode & B_ERASE_FILE) {
			sFileSystem[fPath] = "";
		}

        if (openMode & B_READ_ONLY) {
            if (sFileSystem.find(fPath) == sFileSystem.end()) {
                fInitStatus = B_ENTRY_NOT_FOUND;
                return fInitStatus;
            }
        }

        if ((openMode & B_WRITE_ONLY) || (openMode & B_READ_WRITE)) {
             if (sFileSystem.find(fPath) == sFileSystem.end()) {
                 if (openMode & B_CREATE_FILE) {
                     sFileSystem[fPath] = "";
                 } else {
                     fInitStatus = B_ENTRY_NOT_FOUND;
                     return fInitStatus;
                 }
             }
        }

		fInitStatus = B_OK;
        fPosition = 0;
		return B_OK;
	}

	status_t InitCheck() { return fInitStatus; }

	ssize_t Read(void* buffer, size_t size) {
		if (fInitStatus != B_OK) return B_ERROR;

        std::string& content = sFileSystem[fPath];
        if (fPosition >= content.length()) return 0;

        size_t bytesToRead = size;
        if (fPosition + bytesToRead > content.length())
            bytesToRead = content.length() - fPosition;

        memcpy(buffer, content.c_str() + fPosition, bytesToRead);
        fPosition += bytesToRead;

		return bytesToRead;
	}

	ssize_t Write(const void* buffer, size_t size) {
		if (fInitStatus != B_OK) return B_ERROR;

        std::string& content = sFileSystem[fPath];
        if (fPosition > content.length()) {
            content.resize(fPosition, 0);
        }

        std::string data((const char*)buffer, size);
        if (fPosition == content.length()) {
            content += data;
        } else {
            content.replace(fPosition, size, data);
        }
        fPosition += size;

		return size;
	}

    status_t GetSize(off_t* size) {
        if (fInitStatus != B_OK) return B_ERROR;
        if (sFileSystem.find(fPath) == sFileSystem.end()) return B_ENTRY_NOT_FOUND;
        *size = sFileSystem[fPath].length();
        return B_OK;
    }

private:
	std::string fPath;
	uint32 fOpenMode;
	status_t fInitStatus;
    size_t fPosition;
};

#endif // _FILE_H
