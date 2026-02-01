#ifndef _ENTRY_REF_H
#define _ENTRY_REF_H
#include <string>

struct entry_ref {
    std::string path;
    entry_ref(const char* p = "") : path(p ? p : "") {}
};
#endif
