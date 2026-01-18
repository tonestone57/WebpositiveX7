/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <vector>
#include <time.h>
#include <sys/time.h>

// Mock Headers
#include "String.h"
#include "Message.h"
#include "Handler.h"
#include "Looper.h"
// #include "BookmarkBar.h" // Commented out to avoid linking real BookmarkBar which pulls in UI dependencies

// Mock BookmarkBar for benchmark purposes
class BookmarkBar : public BHandler {
public:
    BookmarkBar() : BHandler("BookmarkBar") {}

    virtual void MessageReceived(BMessage* message) {
        // Simulate processing
        if (message->what == 1234) { // Simulate bookmark loaded message
            // Do some work
            for(volatile int i=0; i<1000; ++i) {}
        }
    }

    void AddItem(const char* title, const char* url) {
        // Simulate adding item
        BString t(title);
        BString u(url);
    }
};

bigtime_t system_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (bigtime_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

void TestBlockingLoad(int count) {
    BookmarkBar bar;
    bigtime_t start = system_time();

    // Simulate blocking load: read file, create item, add item - all in one go
    for (int i = 0; i < count; i++) {
        BMessage msg(1234);
        msg.AddString("title", "Bookmark");
        msg.AddString("url", "http://example.com");

        // Direct processing (Blocking UI)
        bar.MessageReceived(&msg);
        bar.AddItem("Bookmark", "http://example.com");
    }

    bigtime_t end = system_time();
    printf("Blocking Load (%d items): %ld us (UI Frozen)\n", count, end - start);
}

void TestAsyncLoad(int count) {
    BookmarkBar bar;
    bigtime_t start = system_time();

    // Simulate Async Load:
    // 1. Worker thread reads file (simulated delay)
    // 2. Worker sends message to Window (simulated here)
    // 3. Window processes message (add item)

    // Step 1: Simulate reading (fast in memory)
    std::vector<BMessage*> messages;
    for (int i = 0; i < count; i++) {
        BMessage* msg = new BMessage(1234);
        msg->AddString("title", "Bookmark");
        msg->AddString("url", "http://example.com");
        messages.push_back(msg);
    }

    bigtime_t readDone = system_time();

    // Step 2 & 3: Process messages (UI thread work)
    // This is the part that blocks UI. In async model, this happens interleaved or batched.
    // Here we simulate the total time UI is busy.

    bigtime_t msgReceivedStart = system_time();
    for (int i = 0; i < count; i++) {
        bar.MessageReceived(messages[i]);
        bar.AddItem("Bookmark", "http://example.com");
        delete messages[i];
    }
    bigtime_t end = system_time();

    printf("Async Load (%d items): Total %ld us, UI Blocked only %ld us\n",
        count, end - start, end - msgReceivedStart);
}

int main() {
    printf("Benchmark: Blocking vs Async Bookmark Loading\n");
    TestBlockingLoad(100);
    TestAsyncLoad(100);
    return 0;
}
