#ifndef LOCKER_H
#define LOCKER_H

#include <SupportDefs.h>

class BLocker {
public:
	BLocker(const char* name = NULL) {}
	virtual ~BLocker() {}

	bool Lock() { return true; }
	void Unlock() {}
};

#endif
