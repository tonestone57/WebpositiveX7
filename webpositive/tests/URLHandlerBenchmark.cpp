/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <time.h>
#include <vector>
#include <string>

// Mocks
#include <String.h>
#include "Roster.h"

// Mock for be_roster
BRoster* be_roster = new BRoster();

#include "../support/URLHandler.h"
#define _ROSTER_H
#include "../support/URLHandler.cpp"

// Mock for kSearchEngines
const struct SearchEngine kSearchEngines[] = {
	{ "Google", "https://www.google.com/search?q=%s", "g" },
	{ NULL, NULL, NULL }
};

int main()
{
	// Construct a search string with mixed characters
	BString search = "Haiku is an open-source operating system that specifically targets personal computing. ";
	search += "It is inspired by BeOS. URL: http://www.haiku-os.org/ ?query=test&param=123";
    // Make it longer to emphasize loop overhead
    for (int i = 0; i < 5; i++) {
        search += search;
    }

	printf("Benchmarking EncodeURIComponent with string length: %d\n", search.Length());

	clock_t start = clock();
	int iterations = 100000;

	for (int i = 0; i < iterations; i++) {
		BString result = URLHandler::EncodeURIComponent(search);
        // preventative check to ensure compiler doesn't optimize away
        if (result.Length() == 0) printf("Error\n");
	}

	clock_t end = clock();
	double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

	printf("Time taken for %d iterations: %f seconds\n", iterations, time_spent);
    printf("Average time per iteration: %f microseconds\n", (time_spent * 1000000) / iterations);

	return 0;
}
