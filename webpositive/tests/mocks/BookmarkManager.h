#ifndef BOOKMARK_MANAGER_H
#define BOOKMARK_MANAGER_H
#include "SupportDefs.h"
#include "Path.h"

class BookmarkManager {
public:
    static status_t ExportBookmarks(const BPath& path) { return B_OK; }
    static status_t ImportBookmarks(const BPath& path) { return B_OK; }
};
#endif
