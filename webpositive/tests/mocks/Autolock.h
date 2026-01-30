#ifndef _AUTOLOCK_H
#define _AUTOLOCK_H
#include "Locker.h"
class BAutolock {
public:
    BAutolock(BLocker* locker) {}
    BAutolock(BLocker& locker) {}
    bool IsLocked() const { return true; }
};
#endif
