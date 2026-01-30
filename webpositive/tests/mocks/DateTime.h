#ifndef _MOCK_DATE_TIME_H
#define _MOCK_DATE_TIME_H
#include "SupportDefs.h"
#include "Message.h"
#include <time.h>

class BDate {
public:
    BDate() {}
    BDate(const BMessage*) {}
    void AddDays(int) {}
    BString LongDayName() const { return "Monday"; }
    bool operator==(const BDate&) const { return true; }
};

class BTime {
public:
    BTime(int, int, int) {}
};

class BDateTime {
public:
    BDateTime() : t(0) {}
    BDateTime(const BMessage* archive) : t(0) {
        if (archive) {
            int64 val;
            if (archive->FindInt64("mock_time", &val) == B_OK) {
                t = (time_t)val;
            }
        }
    }

    static BDateTime CurrentDateTime(int) { return BDateTime(); }
    time_t Time_t() const { return t; }
    void SetTime_t(time_t time) { t = time; }
    void SetTime(const BTime&) {}

    BDate Date() const { return BDate(); }

    status_t Archive(BMessage* archive) const {
        if (archive) {
            return archive->AddInt64("mock_time", (int64)t);
        }
        return B_ERROR;
    }

    bool operator<(const BDateTime& other) const { return t < other.t; }
    bool operator==(const BDateTime& other) const { return t == other.t; }

private:
    time_t t;
};
enum { B_LOCAL_TIME };
#endif
