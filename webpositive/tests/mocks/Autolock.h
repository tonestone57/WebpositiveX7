#ifndef _AUTOLOCK_H
#define _AUTOLOCK_H

#include "Locker.h"

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
    bool IsLocked() {
        return fLocker && fLocker->IsLocked();
    }

private:
    BLocker* fLocker;
};

#endif
