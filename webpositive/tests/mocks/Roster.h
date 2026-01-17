#ifndef ROSTER_H
#define ROSTER_H
#include "SupportDefs.h"
#include <vector>
#include <string>

class BRoster {
public:
    status_t Launch(const char* type, int argc, const char** argv) {
        // Mock launch: just return B_OK if type starts with application/
        return B_OK;
    }
};
extern BRoster* be_roster;
#endif
