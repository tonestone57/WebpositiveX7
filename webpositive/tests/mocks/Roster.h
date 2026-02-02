#ifndef _MOCK_ROSTER_H
#define _MOCK_ROSTER_H

#include "SupportDefs.h"
#include "Entry.h"

class BMessage;

class BRoster {
public:
    BRoster() {}
    status_t Launch(const char* signature) { return B_OK; }
    status_t Launch(const char* signature, int argc, const char* const* argv) {
        // Simulate failure for localhost pseudo-protocol
        if (signature && strstr(signature, ".localhost")) return B_NAME_NOT_FOUND;
        return B_OK;
    }
    status_t Launch(const entry_ref* ref) { return B_OK; }
    status_t Launch(const char* signature, BMessage* message) { return B_OK; }
};

extern BRoster* be_roster;

#endif
