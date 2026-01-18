/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _AUTOLOCK_H
#define _AUTOLOCK_H

#include "Locker.h"

class BAutolock {
public:
	inline BAutolock(BLocker& locker)
		: fLocker(&locker)
	{
		fLocker->Lock();
	}

	inline BAutolock(BLocker* locker)
		: fLocker(locker)
	{
		if (fLocker) fLocker->Lock();
	}

	inline ~BAutolock()
	{
		if (fLocker) fLocker->Unlock();
	}

	inline bool IsLocked()
	{
		return fLocker && fLocker->IsLocked();
	}

private:
	BLocker* fLocker;
};

#endif // _AUTOLOCK_H
