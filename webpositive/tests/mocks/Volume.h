#ifndef _MOCK_VOLUME_H
#define _MOCK_VOLUME_H
#include "SupportDefs.h"

class BVolume {
public:
    BVolume() {}
    status_t InitCheck() { return B_OK; }
    bool KnowsQuery() { return true; }
};
#endif
