/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <assert.h>
#include <String.h>
#include "Roster.h"

// Mock for be_roster
BRoster* be_roster = new BRoster();

#include "../support/URLHandler.h"
#define _ROSTER_H
#include "../support/URLHandler.cpp"

// Mock for kSearchEngines matching SettingsKeys.cpp format
const struct SearchEngine kSearchEngines[] = {
	{ "Google", "https://www.google.com/search?q=%s", "g" },
	{ "Bing", "https://www.bing.com/search?q=%s", "b" },
	{ NULL, NULL, NULL }
};

int main()
{
	BString outURL;
	BString searchPageURL = "https://duckduckgo.com/?q=%s";
	BString input;
	URLHandler::Action action;
	int result = 0;

	// Test IPv6 Loopback
	input = "[::1]";
	action = URLHandler::CheckURL(input, outURL, searchPageURL);

	if (action == URLHandler::LOAD_URL && outURL == "http://[::1]") {
		printf("Test IPv6 Loopback Passed\n");
	} else {
		printf("Test IPv6 Loopback Failed: Got action %d, URL '%s'\n", action, outURL.String());
		result = 1;
	}

    // Test IPv6 with port
	input = "[::1]:8080";
	action = URLHandler::CheckURL(input, outURL, searchPageURL);

	if (action == URLHandler::LOAD_URL && outURL == "http://[::1]:8080") {
		printf("Test IPv6 Loopback with port Passed\n");
	} else {
		printf("Test IPv6 Loopback with port Failed: Got action %d, URL '%s'\n", action, outURL.String());
		result = 1;
	}

	// Test Invalid IPv6 (brackets with space) -> Search
	input = "[search query]";
	action = URLHandler::CheckURL(input, outURL, searchPageURL);
	// Should go to search engine
	if (action == URLHandler::LOAD_URL && outURL.FindFirst("duckduckgo") != B_ERROR) {
		printf("Test Invalid IPv6 (space) Passed\n");
	} else {
		printf("Test Invalid IPv6 (space) Failed: Got action %d, URL '%s'\n", action, outURL.String());
		result = 1;
	}

	return result;
}
