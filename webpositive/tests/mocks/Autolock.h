/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef AUTOLOCK_H
#define AUTOLOCK_H

#include <Locker.h>

class BAutolock {
public:
	BAutolock(BLocker* locker) : fLocker(locker) {
		if (fLocker) fLocker->Lock();
	}
	BAutolock(BLocker& locker) : fLocker(&locker) {
		if (fLocker) fLocker->Lock();
	}
	~BAutolock() {
		if (fLocker) fLocker->Unlock();
	}
	bool IsLocked() { return fLocker != NULL; }

private:
	BLocker* fLocker;
};

#endif // AUTOLOCK_H
