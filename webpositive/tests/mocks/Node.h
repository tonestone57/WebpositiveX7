#ifndef _MOCK_NODE_H
#define _MOCK_NODE_H
#include "Entry.h"
#include "SupportDefs.h"

class BNode {
public:
    BNode() {}
    BNode(const BEntry* entry) {}
    virtual ~BNode() {}
    status_t GetNextAttrName(char* buffer) { return B_ENTRY_NOT_FOUND; }
};
#endif
