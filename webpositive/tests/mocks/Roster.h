#ifndef _MOCK_ROSTER_H
#define _MOCK_ROSTER_H

#include "SupportDefs.h"
#include "Entry.h"

class BRoster {
public:
    BRoster() {}
    status_t Launch(const char* signature) { return B_OK; }
    status_t Launch(const char* signature, int argc, const char* const* argv) { return B_OK; }
    status_t Launch(const entry_ref* ref) { return B_OK; }
};

extern BRoster* be_roster;

#endif
