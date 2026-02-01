#ifndef _MOCK_NODE_H
#define _MOCK_NODE_H
#include "Entry.h"
#include "SupportDefs.h"
#include "String.h"

class BNode {
public:
    std::map<std::string, std::string> fAttributes;

    BNode() {}
    BNode(const BEntry* entry) {
        if (entry) fAttributes = entry->fAttributes;
    }
    virtual ~BNode() {}
    status_t GetNextAttrName(char* buffer) { return B_ENTRY_NOT_FOUND; }

    status_t ReadAttrString(const char* name, BString* result) {
        if (fAttributes.count(name)) {
             if (result) *result = fAttributes[name].c_str();
             return B_OK;
        }
        return B_ENTRY_NOT_FOUND;
    }
};
#endif
