#include <stdio.h>
#include <stdlib.h>
#include <new>
#include <vector>
#include <string>
#include <map>
#include <sys/time.h>

// Memory tracking
size_t gCurrentMemoryUsage = 0;
size_t gPeakMemoryUsage = 0;
size_t gMaxSingleAllocation = 0;

void* operator new(size_t size) {
    if (size == 0) size = 1;
    size_t actualSize = size + sizeof(size_t);
    size_t* ptr = (size_t*)malloc(actualSize);
    if (!ptr) throw std::bad_alloc();
    *ptr = size;

    gCurrentMemoryUsage += size;
    if (gCurrentMemoryUsage > gPeakMemoryUsage) gPeakMemoryUsage = gCurrentMemoryUsage;
    if (size > gMaxSingleAllocation) gMaxSingleAllocation = size;

    return ptr + 1;
}

void* operator new(size_t size, const std::nothrow_t&) noexcept {
    try {
        return operator new(size);
    } catch (...) {
        return nullptr;
    }
}

void* operator new[](size_t size) {
    return operator new(size);
}

void* operator new[](size_t size, const std::nothrow_t&) noexcept {
    return operator new(size, std::nothrow);
}

void operator delete(void* ptr) noexcept {
    if (!ptr) return;
    size_t* realPtr = (size_t*)ptr - 1;
    size_t size = *realPtr;
    if (gCurrentMemoryUsage >= size)
        gCurrentMemoryUsage -= size;
    free(realPtr);
}

void operator delete[](void* ptr) noexcept {
    operator delete(ptr);
}

// Mock Headers
#include "mocks/String.h"
#include "mocks/DateTime.h"
#include "mocks/Locker.h"
#include "mocks/Handler.h"
#include "mocks/Message.h"
#include "mocks/Autolock.h"
#include "mocks/Entry.h"
#include "mocks/File.h"
#include "mocks/FindDirectory.h"
#include "mocks/MessageRunner.h"
#include "mocks/Path.h"
#include "mocks/Messenger.h"
#include "mocks/BrowserApp.h"
#include "mocks/OS.h"
#include "mocks/MockFileSystem.h"

// Define static content for BFile mock
std::string BFile::content = "";

// Define MockFileSystem statics
std::map<std::string, MockEntryData> MockFileSystem::sEntries;
long MockFileSystem::sGetNextEntryCount = 0;
long MockFileSystem::sOpenCount = 0;
long MockFileSystem::sReadAttrCount = 0;

// Stub for find_directory
status_t find_directory(directory_which which, BPath* path) {
    return B_OK;
}

// Stub for spawn_thread/resume_thread (mock threading)
thread_id spawn_thread(status_t (*func)(void*), const char* name, int32 priority, void* data) {
    func(data);
    return 1;
}

status_t resume_thread(thread_id thread) {
    return B_OK;
}

status_t kill_thread(thread_id thread) {
    return B_OK;
}

int32_t atomic_add(int32_t* value, int32_t addvalue) {
    int32_t old = *value;
    *value += addvalue;
    return old;
}

int32_t atomic_get(int32_t* value) {
    return *value;
}

void snooze(bigtime_t microseconds) {}

// Include the source file under test
#define _AUTOLOCK_H
#define _ENTRY_H
#define _FILE_H
#define _FIND_DIRECTORY_H
#define _MESSAGE_H
#define _MESSAGE_RUNNER_H
#define _PATH_H
#include "../BrowsingHistory.cpp"

void GenerateHistoryFile(size_t targetSize) {
    BFile::content.reserve(targetSize + 1024);
    BFile::content = "URL,Date,Count\n";

    int i = 0;
    while (BFile::content.length() < targetSize) {
        char buffer[512];
        // Mix of normal and quoted URLs
        if (i % 5 == 0) {
            snprintf(buffer, sizeof(buffer), "\"http://example.com/page?id=%d,tag=foo\",2023-10-27 10:00:00,%d\n", i, i % 100);
        } else {
            snprintf(buffer, sizeof(buffer), "http://example.com/page/%d,2023-10-27 10:00:00,%d\n", i, i % 100);
        }
        BFile::content.append(buffer);
        i++;
    }
}

int main() {
    printf("Running ImportHistory Benchmark...\n");

    // Generate ~10MB file
    size_t targetSize = 10 * 1024 * 1024;
    GenerateHistoryFile(targetSize);
    printf("Generated mock file size: %zu bytes\n", BFile::content.length());

    // Reset stats
    gPeakMemoryUsage = 0;
    gMaxSingleAllocation = 0;
    gCurrentMemoryUsage = 0;

    struct timeval start, end;
    gettimeofday(&start, NULL);

    BPath dummyPath("/boot/home/config/settings/WebPositive/BrowsingHistory");
    status_t result = BrowsingHistory::ImportHistory(dummyPath);

    gettimeofday(&end, NULL);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    if (result != B_OK) {
        printf("ImportHistory failed with status: %d\n", result);
        return 1;
    }

    int32 count = BrowsingHistory::DefaultInstance()->CountItems();
    printf("Imported %d items.\n", count);

    printf("Time: %.4f s\n", elapsed);
    printf("Peak Memory: %zu bytes (%.2f MB)\n", gPeakMemoryUsage, gPeakMemoryUsage / (1024.0 * 1024.0));
    printf("Max Single Allocation: %zu bytes (%.2f MB)\n", gMaxSingleAllocation, gMaxSingleAllocation / (1024.0 * 1024.0));

    return 0;
}
