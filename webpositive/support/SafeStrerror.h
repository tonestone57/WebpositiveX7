/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SAFE_STRERROR_H
#define SAFE_STRERROR_H

#include <string.h>

class SafeStrerror {
public:
	SafeStrerror(int error)
	{
		memset(fBuffer, 0, sizeof(fBuffer));
		// Note: We ignore the return value of strerror_r here because we
		// handle both POSIX (int) and GNU (char*) versions via overloading
		// or just assuming POSIX for Haiku.
		// However, to be robust against GNU version returning a pointer and NOT
		// filling buffer:
		// But in C++, we can't easily ignore different return types in one expression.
		// Haiku uses POSIX strerror_r returning int.
		if (strerror_r(error, fBuffer, sizeof(fBuffer)) != 0) {
			// If it failed, we might want to put the error code in the string.
			// But POSIX says valid error message is put in buffer even on truncation (ERANGE).
			// If invalid error number (EINVAL), it puts "Unknown error..." usually.
			// So checking return value might be redundant or tricky if it returns char*.
			// Let's assume best effort.
			// Ensure null termination.
			fBuffer[sizeof(fBuffer) - 1] = '\0';
		}
	}

	const char* String() const
	{
		return fBuffer;
	}

private:
	char fBuffer[256];
};

#endif // SAFE_STRERROR_H
