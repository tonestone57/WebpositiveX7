#ifndef _MOCK_NODE_H
#define _MOCK_NODE_H
#include "Entry.h"
#include "SupportDefs.h"
#include "String.h"
#include "MockFileSystem.h"

class BNode {
public:
    std::map<std::string, std::string> fAttributes;

    BNode() {}
    BNode(const BEntry* entry) {
        if (entry) fAttributes = entry->fAttributes;
    }
    BNode(const entry_ref* ref) {
        if (ref) {
             MockEntryData data;
             if (MockFileSystem::GetEntry(ref->path, &data)) {
                 fAttributes = data.attributes;
             }
        }
    }
    virtual ~BNode() {}

    status_t InitCheck() { return B_OK; }

    status_t GetNextAttrName(char* buffer) { return B_ENTRY_NOT_FOUND; }

    status_t ReadAttrString(const char* name, BString* result) {
        MockFileSystem::sReadAttrCount++;
        if (fAttributes.count(name)) {
             if (result) *result = fAttributes[name].c_str();
             return B_OK;
        }
        return B_ENTRY_NOT_FOUND;
    }
};
#endif
