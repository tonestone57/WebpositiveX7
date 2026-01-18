#ifndef LOCKER_H
#define LOCKER_H

#include <SupportDefs.h>
#include <stddef.h>

class BLocker {
public:
	BLocker(const char* name = NULL) {}
	virtual ~BLocker() {}

	bool Lock() { return true; }
	void Unlock() {}
    bool IsLocked() const { return true; }
};

#endif
