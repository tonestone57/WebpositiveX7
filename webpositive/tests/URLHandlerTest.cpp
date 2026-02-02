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
// We remove the trailing spaces here to test the robust logic in URLHandler.
const struct SearchEngine kSearchEngines[] = {
	{ "Google", "https://www.google.com/search?q=%s", "g" },
	{ "Bing", "https://www.bing.com/search?q=%s", "b" },
	{ NULL, NULL, NULL }
};

int main()
{
	BString outURL;
	BString searchPageURL = "https://duckduckgo.com/?q=%s";

	// Test 1: Standard URL (http)
	BString input = "http://www.haiku-os.org";
	URLHandler::Action action = URLHandler::CheckURL(input, outURL, searchPageURL);
	assert(action == URLHandler::LOAD_URL);
	assert(outURL == "http://www.haiku-os.org");
	printf("Test 1 Passed: Standard URL (http)\n");

	// Test 2: Standard URL (https)
	input = "https://www.haiku-os.org";
	action = URLHandler::CheckURL(input, outURL, searchPageURL);
	assert(action == URLHandler::LOAD_URL);
	assert(outURL == "https://www.haiku-os.org");
	printf("Test 2 Passed: Standard URL (https)\n");

	// Test 3: URL without protocol (www.domain.com) -> should add http://
	input = "www.haiku-os.org";
	action = URLHandler::CheckURL(input, outURL, searchPageURL);
	assert(action == URLHandler::LOAD_URL);
	assert(outURL == "http://www.haiku-os.org");
	printf("Test 3 Passed: URL without protocol\n");

	// Test 4: Localhost
	input = "localhost";
	action = URLHandler::CheckURL(input, outURL, searchPageURL);
	assert(action == URLHandler::LOAD_URL);
	assert(outURL == "http://localhost/");
	printf("Test 4 Passed: Localhost\n");

	// Test 5: Search Query
	input = "Haiku OS";
	action = URLHandler::CheckURL(input, outURL, searchPageURL);
	assert(action == URLHandler::LOAD_URL);
	assert(outURL == "https://duckduckgo.com/?q=Haiku%20OS");
	printf("Test 5 Passed: Search Query\n");

	// Test 6: Search Engine Shortcut (Standard usage)
	input = "g Haiku";
	action = URLHandler::CheckURL(input, outURL, searchPageURL);
	assert(action == URLHandler::LOAD_URL);
	if (outURL != "https://www.google.com/search?q=Haiku") {
        printf("Test 6 Failed: Expected 'https://www.google.com/search?q=Haiku', Got '%s'\n", outURL.String());
    }
	assert(outURL == "https://www.google.com/search?q=Haiku");
	printf("Test 6 Passed: Search Engine Shortcut\n");

	// Test 7: Search Engine Shortcut (Exact match "g")
	input = "g";
	action = URLHandler::CheckURL(input, outURL, searchPageURL);
	assert(action == URLHandler::LOAD_URL);
	// Expect empty query or handled?
    // "https://www.google.com/search?q="
	if (outURL != "https://www.google.com/search?q=") {
        printf("Test 7 Failed: Expected 'https://www.google.com/search?q=', Got '%s'\n", outURL.String());
    }
    // We accept strict prefix match which makes query empty.
	assert(outURL == "https://www.google.com/search?q=");
	printf("Test 7 Passed: Search Engine Shortcut (Exact)\n");

    // Test 8: False Positive ("good") -> Should NOT use Google
	input = "good";
	action = URLHandler::CheckURL(input, outURL, searchPageURL);
	assert(action == URLHandler::LOAD_URL);
	// Should go to default search (DuckDuckGo) with query "good"
	if (outURL != "https://duckduckgo.com/?q=good") {
        printf("Test 8 Failed: Expected 'https://duckduckgo.com/?q=good', Got '%s'\n", outURL.String());
    }
	assert(outURL == "https://duckduckgo.com/?q=good");
	printf("Test 8 Passed: False Positive Avoidance\n");

	// Test 9: FTP URL
	input = "ftp://ftp.haiku-os.org";
	action = URLHandler::CheckURL(input, outURL, searchPageURL);
	assert(action == URLHandler::LAUNCH_APP);
	printf("Test 9 Passed: FTP URL (Delegated)\n");

	// Test 10: Localhost with port
	input = "localhost:8080";
	action = URLHandler::CheckURL(input, outURL, searchPageURL);
	assert(action == URLHandler::LOAD_URL);
	if (outURL != "http://localhost:8080") {
		printf("Test 10 Failed: Expected 'http://localhost:8080', Got '%s'\n", outURL.String());
	}
	assert(outURL == "http://localhost:8080");
	printf("Test 10 Passed: Localhost with port\n");


	printf("All tests passed!\n");
	return 0;
}
