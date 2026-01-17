/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <assert.h>
#include "String.h"
#include "Roster.h"

// Mock for be_roster
BRoster* be_roster = new BRoster();

#include "../support/URLHandler.h"
#include "../support/URLHandler.cpp"

// Mock for kSearchEngines matching SettingsKeys.cpp format (with spaces)
const struct SearchEngine kSearchEngines[] = {
	{ "Google", "https://www.google.com/search?q=%s", "g " },
	{ "Bing", "https://www.bing.com/search?q=%s", "b " },
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

	// Test 6: Search Engine Shortcut
	input = "g Haiku";
	action = URLHandler::CheckURL(input, outURL, searchPageURL);
	assert(action == URLHandler::LOAD_URL);
    if (!(outURL == "https://www.google.com/search?q=Haiku")) {
        printf("Test 6 Failed: Expected 'https://www.google.com/search?q=Haiku', Got '%s'\n", outURL.String());
    }
	assert(outURL == "https://www.google.com/search?q=Haiku");
	printf("Test 6 Passed: Search Engine Shortcut\n");

	// Test 7: IP Address
	input = "127.0.0.1";
	action = URLHandler::CheckURL(input, outURL, searchPageURL);
	assert(action == URLHandler::LOAD_URL);
	assert(outURL == "http://127.0.0.1");
	printf("Test 7 Passed: IP Address\n");

	printf("All tests passed!\n");
	return 0;
}
