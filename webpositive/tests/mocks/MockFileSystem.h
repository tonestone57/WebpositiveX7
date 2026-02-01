#ifndef _MOCK_FILE_SYSTEM_H
#define _MOCK_FILE_SYSTEM_H

#include <map>
#include <string>
#include <vector>
#include <iostream>

struct MockEntryData {
    std::string name;
    std::string path; // Full path
    bool isDirectory;
    std::map<std::string, std::string> attributes;
    std::string content;
};

class MockFileSystem {
public:
    static std::map<std::string, MockEntryData> sEntries;
    static long sGetNextEntryCount;
    static long sOpenCount;
    static long sReadAttrCount;

    static void Reset() {
        sEntries.clear();
        sGetNextEntryCount = 0;
        sOpenCount = 0;
        sReadAttrCount = 0;
    }

    static void AddEntry(const std::string& path, const MockEntryData& data) {
        sEntries[path] = data;
    }

    static bool GetEntry(const std::string& path, MockEntryData* data) {
        auto it = sEntries.find(path);
        if (it != sEntries.end()) {
            if (data) *data = it->second;
            return true;
        }
        return false;
    }
};

// Define static members in the header for simplicity in this mock environment,
// or usually in a .cpp, but we don't have a separate lib for mocks.
// We will rely on one test file including this or defining these.
// To be safe, we can use inline or declare here and define in the test file.
// For now, I'll assume I can define them in the test file or use a separate cpp if needed.
// But to avoid linker errors if multiple tests include this, I should probably declare them extern here.

#endif
