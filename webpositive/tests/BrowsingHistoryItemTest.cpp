/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <new>

// System Headers
#include <String.h>
#include <DateTime.h>
#include <Locker.h>
#include <Handler.h>
#include <Message.h>
#include <Autolock.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <MessageRunner.h>
#include <Path.h>
#include <Messenger.h>
#include <OS.h>

#include "BrowserApp.h"
#include "BrowsingHistory.h"

int main()
{
	printf("Running BrowsingHistoryItem Tests...\n");

	// Test Construction
	BString url1("http://www.haiku-os.org");
	BrowsingHistoryItem item1(url1);
	assert(item1.URL() == url1);
	assert(item1.InvokationCount() == 0);
	printf("Test 1 Passed: Construction\n");

	// Test Copy
	BrowsingHistoryItem item2(item1);
	assert(item2 == item1);
	printf("Test 2 Passed: Copy Construction and Equality\n");

	// Test Invocation
	item1.Invoked();
	assert(item1.InvokationCount() == 1);
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

	printf("All BrowsingHistoryItem tests passed!\n");
	return 0;
}
