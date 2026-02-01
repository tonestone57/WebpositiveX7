#ifndef _MOCK_QUERY_H
#define _MOCK_QUERY_H
#include "SupportDefs.h"
#include "Entry.h"
#include "Volume.h"
#include "MockFileSystem.h"
#include <vector>
#include <string>

enum query_op {
    B_EQ,
    B_GT,
    B_GE,
    B_LT,
    B_LE,
    B_NE,
    B_CONTAINS,
    B_BEGINS_WITH,
    B_ENDS_WITH,
    B_AND,
    B_OR,
    B_NOT
};

class BQuery {
public:
    std::vector<entry_ref> fResults;
    size_t fIterIndex;
    std::string fAttr;
    std::string fValue;
    query_op fOp;

    BQuery() : fIterIndex(0) {}

    status_t SetVolume(const BVolume* vol) { return B_OK; }
    status_t SetPredicate(const char* pred) { return B_OK; }

    void PushAttr(const char* attr) { fAttr = attr; }
    void PushString(const char* str) { fValue = str; }
    void PushOp(query_op op) { fOp = op; }

    status_t Fetch() {
        fResults.clear();
        fIterIndex = 0;
        // Naive scan of MockFileSystem
        for (auto const& [path, data] : MockFileSystem::sEntries) {
             if (data.isDirectory) continue;

             if (!fAttr.empty()) {
                 if (data.attributes.count(fAttr)) {
                     // Simple match: if op is EQ and value is "*", return match
                     // Or check actual value
                     if (fOp == B_EQ && fValue == "*") {
                         fResults.push_back(entry_ref(path.c_str()));
                     } else if (fOp == B_EQ && data.attributes.at(fAttr) == fValue) {
                         fResults.push_back(entry_ref(path.c_str()));
                     }
                 }
             }
        }
        return B_OK;
    }

    status_t GetNextEntry(BEntry* entry) {
        if (fIterIndex < fResults.size()) {
            if (entry) entry->SetTo(fResults[fIterIndex++].path.c_str());
            return B_OK;
        }
        return B_ENTRY_NOT_FOUND;
    }

    status_t GetNextRef(entry_ref* ref) {
        if (fIterIndex < fResults.size()) {
            if (ref) *ref = fResults[fIterIndex++];
            return B_OK;
        }
        return B_ENTRY_NOT_FOUND;
    }
};
#endif
