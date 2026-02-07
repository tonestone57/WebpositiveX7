/*
 * Benchmark/Mock test for BookmarkBar loading optimization.
 * This test simulates the cost of loading bookmarks synchronously vs asynchronously.
 *
 * Note: This file requires Haiku system headers to compile.
 * It serves as documentation and a verification plan for the optimization.
 */

#include <vector>
#include <stdio.h>
#include <OS.h>

// Mock structures
struct entry_ref {
    char name[256];
};

struct BMenuItem {
    // Mock UI Object
};

struct BookmarkItem {
    int64 inode;
    BMenuItem* item;
};

// Simulated I/O cost (e.g. 2ms per item)
void SimulateIO() {
    snooze(2000);
}

// Simulated CreateBookmarkItem (Heavy operation)
BMenuItem* CreateBookmarkItemMock(int id) {
    SimulateIO();
    return new BMenuItem();
}

// Scenario 1: Blocking Load (Original)
void TestBlockingLoad(int count) {
    bigtime_t start = system_time();

    // UI Thread blocks here
    for (int i = 0; i < count; i++) {
        // Iterate directory
        SimulateIO(); // Directory iteration cost

        // Create Item (I/O heavy)
        BMenuItem* item = CreateBookmarkItemMock(i);

        // Add to UI (Fast)
        // AddItem(item);
        delete item;
    }

    bigtime_t end = system_time();
    printf("Blocking Load (%d items): %lld us (UI Frozen)\n", count, end - start);
}

// Scenario 2: Async Load (Optimized)
void TestAsyncLoad(int count) {
    bigtime_t start = system_time();

    // Background Thread
    std::vector<BookmarkItem*>* items = new std::vector<BookmarkItem*>();
    for (int i = 0; i < count; i++) {
        SimulateIO(); // Directory iteration
        BMenuItem* item = CreateBookmarkItemMock(i);
        BookmarkItem* data = new BookmarkItem{ (int64)i, item };
        items->push_back(data);
    }

    // Main Thread receives message
    bigtime_t msgReceivedStart = system_time();

    for (size_t i = 0; i < items->size(); i++) {
        BookmarkItem* data = (*items)[i];
        // Add to UI (Fast)
        // AddItem(data->item);
        delete data->item;
        delete data;
    }
    delete items;

    bigtime_t end = system_time();

    printf("Async Load (%d items): Total %lld us, UI Blocked only %lld us\n",
        count, end - start, end - msgReceivedStart);
}

int main() {
    printf("Benchmark: Blocking vs Async Bookmark Loading\n");
    TestBlockingLoad(100);
    TestAsyncLoad(100);
    return 0;
}
