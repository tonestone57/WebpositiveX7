#ifndef _MOCK_NODE_INFO_H
#define _MOCK_NODE_INFO_H
#include "SupportDefs.h"
#include "Bitmap.h"
#include "Node.h"

enum icon_size { B_MINI_ICON, B_LARGE_ICON };

class BNodeInfo {
public:
    BNodeInfo(BNode* node) {}
    status_t SetType(const char* type) { return B_OK; }
    status_t SetIcon(const BBitmap* icon, icon_size which) { return B_OK; }
};
#endif
