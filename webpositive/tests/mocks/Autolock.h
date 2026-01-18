#ifndef AUTOLOCK_H
#define AUTOLOCK_H

#include "Locker.h"

class BAutolock {
public:
	BAutolock(BLocker* locker) : fLocker(locker) { if (fLocker) fLocker->Lock(); }
	BAutolock(BLocker& locker) : fLocker(&locker) { fLocker->Lock(); }
	~BAutolock() { if (fLocker) fLocker->Unlock(); }

	bool IsLocked() const { return fLocker != NULL; }

private:
	BLocker* fLocker;
};

#endif // AUTOLOCK_H
