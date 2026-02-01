#include "MockFileSystem.h"

std::map<std::string, MockEntryData> MockFileSystem::sEntries;
long MockFileSystem::sGetNextEntryCount = 0;
long MockFileSystem::sOpenCount = 0;
long MockFileSystem::sReadAttrCount = 0;
