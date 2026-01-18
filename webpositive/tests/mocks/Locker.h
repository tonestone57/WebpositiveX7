#ifndef _LOCKER_H
#define _LOCKER_H

class BLocker {
public:
    BLocker(const char* name) {}
    bool Lock() { return true; }
    void Unlock() {}
    bool IsLocked() const { return true; }
};

#endif
