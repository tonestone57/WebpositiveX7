/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <new>

// Mock Headers
#include "String.h"
#include "DateTime.h"
#include "Locker.h"
#include "Handler.h"
#include "Message.h"
#include "Autolock.h"
#include "Entry.h"
#include "File.h"
#include "FindDirectory.h"
#include "MessageRunner.h"
#include "Path.h"
#include "Messenger.h"
#include "BrowserApp.h"
#include "OS.h"

// Define static content for BFile mock
std::string BFile::content = "";

// Stub for find_directory
status_t find_directory(directory_which which, BPath* path) {
    return B_OK;
}

// Stub for spawn_thread/resume_thread (mock threading)
thread_id spawn_thread(status_t (*func)(void*), const char* name, int32 priority, void* data) {
    // Execute immediately for testing
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

int main()
{
	printf("Running BrowsingHistoryItem Tests via Source Inclusion...\n");

	// Test Construction
	BString url1("http://www.haiku-os.org");
	BrowsingHistoryItem item1(url1);
	assert(item1.URL() == url1);
	assert(item1.InvocationCount() == 0);
	printf("Test 1 Passed: Construction\n");

	// Test Copy
	BrowsingHistoryItem item2(item1);
	assert(item2 == item1);
	printf("Test 2 Passed: Copy Construction and Equality\n");

	// Test Invocation
	item1.Invoked();
	assert(item1.InvocationCount() == 1);
	assert(item1 != item2); // Count changed
	printf("Test 3 Passed: Invocation\n");

	// Test Sorting
	// Create items with different URLs (Date is same due to mock)
	BString urlA("http://a.com");
	BString urlB("http://b.com");
	BrowsingHistoryItem itemA(urlA);
	BrowsingHistoryItem itemB(urlB);

	// itemA < itemB because "a.com" < "b.com"
	assert(itemA < itemB);
	assert(itemB > itemA);
	printf("Test 4 Passed: Sorting by URL\n");

	// Test Serialization (New Format)
	{
		BString url("http://www.haiku-os.org");
		BrowsingHistoryItem item(url);
		item.SetInvocationCount(5);
		// Set a specific time (e.g. 1000 seconds since epoch)
		BDateTime dt;
		dt.SetTime_t(1000);
		item.SetDateTime(dt);

		BMessage archive;
		assert(item.Archive(&archive) == B_OK);

		// Verify fields
		int64 t;
		assert(archive.FindInt64("date_val", &t) == B_OK);
		assert(t == 1000);

		BMessage sub;
		assert(archive.FindMessage("date time", &sub) != B_OK); // Should NOT have old format

		// Unarchive
		BrowsingHistoryItem item2(&archive);
		assert(item2 == item);
		assert(item2.DateTime().Time_t() == 1000);
		printf("Test 5 Passed: Serialization (New Format)\n");
	}

	// Test Serialization (Legacy Format)
	{
		BString url("http://old.com");
		BMessage archive;
		archive.AddString("url", url);
		archive.AddUInt32("invokations", 3);

		BDateTime dt;
		dt.SetTime_t(2000);
		BMessage dtMsg;
		dt.Archive(&dtMsg); // Uses BDateTime::Archive
		archive.AddMessage("date time", &dtMsg);

		BrowsingHistoryItem item(&archive);
		assert(item.URL() == url);
		assert(item.InvocationCount() == 3);
		assert(item.DateTime().Time_t() == 2000);
		printf("Test 6 Passed: Serialization (Legacy Format)\n");
	}

	printf("All BrowsingHistoryItem tests passed!\n");
	return 0;
}
