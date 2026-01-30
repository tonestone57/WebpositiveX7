#ifndef BROWSING_HISTORY_H
#define BROWSING_HISTORY_H
#include "SupportDefs.h"
#include "Path.h"

class BrowsingHistory {
public:
    static status_t ExportHistory(const BPath& path) { return B_OK; }
    static status_t ImportHistory(const BPath& path) { return B_OK; }
};
#endif
