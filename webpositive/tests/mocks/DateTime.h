#ifndef _DATETIME_H
#define _DATETIME_H

#include <time.h>
#include <SupportDefs.h>

class BMessage;

enum time_type {
    B_LOCAL_TIME
};

class BDate {
public:
    void AddDays(int32 days) {}
};

class BDateTime {
public:
    BDateTime() : fTime(0) {}
    BDateTime(const BDateTime& other) : fTime(other.fTime) {}
    BDateTime(const void* archive) : fTime(0) {} // Mock archive constructor
    BDateTime(const BMessage* archive) : fTime(0) {}

    static BDateTime CurrentDateTime(time_type type) {
        BDateTime dt;
        dt.fTime = time(NULL);
        return dt;
    }

    time_t Time_t() const { return fTime; }
    void SetTime_t(time_t t) { fTime = t; }

    bool operator<(const BDateTime& other) const { return fTime < other.fTime; }
    bool operator==(const BDateTime& other) const { return fTime == other.fTime; }

    status_t Archive(BMessage* archive) const { return B_OK; } // Mock Archive

    BDate Date() { return BDate(); }

private:
    time_t fTime;
};

#endif
