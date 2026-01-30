#ifndef _MOCK_LOCKER_H
#define _MOCK_LOCKER_H
class BLocker {
public:
    BLocker(const char* name = NULL) {}
    virtual ~BLocker() {}
    bool Lock() { return true; }
    void Unlock() {}
    bool IsLocked() const { return true; }
};
#endif
