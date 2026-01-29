/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "URLHandler.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <Roster.h>
#include <Url.h>

#include "SettingsKeys.h"

static const char* kHandledProtocols[] = {
	"http",
	"https",
	"file",
	"about",
	"data",
	"gopher"
};

/*static*/ bool
URLHandler::IsValidDomainChar(char ch)
{
	// Characters that are allowed in domain names (IDNA 2008 / LDH rule).
	// We also allow characters that are valid in the authority part of a URL
	// (userinfo, port, IPv6 literals) because this function is used to
	// validate the entire authority string.
	// We deliberately disallow underscore (_) and percent (%) to better
	// distinguish search queries from URLs, as these are not valid in
	// standard hostnames.

	if ((unsigned char)ch > 0x7f)
		return true;

	if (isalnum(ch))
		return true;

	return ch == '-' || ch == '.' || ch == ':'
		|| ch == '[' || ch == ']' || ch == '@';
}

/*static*/ BString
URLHandler::_EncodeURIComponent(const BString& search)
{
	// We have to take care of some of the escaping before we hand over the
	// search string to WebKit, if we want queries like "4+3" to not be
	// searched as "4 3".
	const BString escCharList = " $&`:<>[]{}\"+#%@/;=?\\^|~\',";
	BString result;
	char hexcode[4];

	for (int32 i = 0; i < search.Length(); i++) {
		char c = search[i];
		if (escCharList.FindFirst(c) != B_ERROR) {
			sprintf(hexcode, "%%%02X", (unsigned int)(unsigned char)c);
			result << hexcode;
		} else {
			result << c;
		}
	}

	return result;
}

/*static*/ void
URLHandler::_VisitSearchEngine(const BString& search, BString& outURL, const BString& searchPageURL)
{
	BString searchQuery = search;

	BString searchPrefix;
	search.CopyCharsInto(searchPrefix, 0, 2);

	// Default search URL
	BString engine(searchPageURL);

	// Check if the string starts with one of the search engine shortcuts
	for (int32 i = 0; kSearchEngines[i].url != NULL; i++) {
		if (kSearchEngines[i].shortcut == searchPrefix) {
			engine = kSearchEngines[i].url;
			searchQuery.Remove(0, 2);
			break;
		}
	}

	engine.ReplaceAll("%s", _EncodeURIComponent(searchQuery));
	outURL = engine;
}

/*static*/ URLHandler::Action
URLHandler::CheckURL(const BString& input, BString& outURL, const BString& searchPageURL)
{
	if (input.Length() == 0)
		return DO_NOTHING;

	// First test if the URL has a protocol field
	int32 at = input.FindFirst(":");

	if (at != B_ERROR) {
		// There is a protocol, let's see if we can handle it
		BString proto;
		input.CopyInto(proto, 0, at);

		bool handled = false;

		// First try the built-in supported ones
		for (int32 i = 0; i < (int32)(sizeof(kHandledProtocols) / sizeof(char*));
				i++) {
			handled = (proto == kHandledProtocols[i]);
			if (handled)
				break;
		}

		if (handled) {
			// This is the easy case, a complete and well-formed URL, we can
			// navigate to it without further efforts.
			outURL = input;
			return LOAD_URL;
		} else {
			// There is what looks like a protocol, but one we don't know.
			// Ask the BRoster if there is a matching filetype and app which
			// can handle it.
			BString temp;
			temp = "application/x-vnd.Be.URL.";
			temp += proto;

			const char* argv[] = { input.String(), NULL };

			if (be_roster->Launch(temp.String(), 1, argv) == B_OK)
				return LAUNCH_APP;
		}
	}

	// There is no protocol or only an unsupported one. So let's try harder to
	// guess what the request is.

	// "localhost" is a special case, it is a valid domain name but has no dots.
	// Handle it separately.
	if (input == "localhost") {
		outURL = "http://localhost/";
		return LOAD_URL;
	} else {
		// Also handle URLs starting with "localhost" followed by a path.
		const char* localhostPrefix = "localhost/";

		if (input.Compare(localhostPrefix, strlen(localhostPrefix)) == 0) {
			if (input.FindFirst("://") == B_ERROR)
				outURL = BString("http://") << input;
			else
				outURL = input;
			return LOAD_URL;
		} else {
			// In all other cases we try to detect a valid domain name. There
			// must be at least one dot and no spaces until the first / in the
			// URL.
			bool isURL = false;

			for (int32 i = 0; i < input.Length(); i++) {
				if (input[i] == '.')
					isURL = true;
				else if (input[i] == '/' || input[i] == '?' || input[i] == '#')
					break;
				else if (!IsValidDomainChar(input[i])) {
					isURL = false;
					break;
				}
			}

			if (isURL) {
				// This is apparently an URL missing the protocol part. In that
				// case we default to http.
				BString prefixed = "http://";
				prefixed << input;
				outURL = prefixed;
				return LOAD_URL;
			} else {
				// We couldn't find anything that looks like an URL. Let's
				// assume what we have is a search request and go to the search
				// engine.
				_VisitSearchEngine(input, outURL, searchPageURL);
				return LOAD_URL;
			}
		}
	}
	return DO_NOTHING;
}
