/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "URLHandler.h"

#include <stdio.h>
#include <string.h>
#include <Roster.h>
#include <ctype.h>
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

	if (isalnum((unsigned char)ch))
		return true;

	return ch == '-' || ch == '.' || ch == ':'
		|| ch == '[' || ch == ']' || ch == '@';
}


static bool
IsValidScheme(const BString& scheme)
{
	if (scheme.Length() == 0)
		return false;

	if (!isalpha((unsigned char)scheme[0]))
		return false;

	for (int32 i = 1; i < scheme.Length(); i++) {
		unsigned char c = (unsigned char)scheme[i];
		if (!isalnum(c) && c != '+' && c != '-' && c != '.')
			return false;
	}
	return true;
}


static bool
ShouldEscape(char c)
{
	// We have to take care of some of the escaping before we hand over the
	// search string to WebKit, if we want queries like "4+3" to not be
	// searched as "4 3".
	if (isalnum((unsigned char)c))
		return false;

	switch (c) {
		case '-':
		case '_':
		case '.':
		case '~':
			return false;
		default:
			return true;
	}
}


/*static*/ BString
URLHandler::EncodeURIComponent(const BString& search)
{
	int32 length = search.Length();
	BString result;

	// Worst case expansion: every character is escaped (3 bytes)
	// Optimization: Single pass with pessimistic allocation
	char* buffer = result.LockBuffer(length * 3);
	if (buffer == NULL)
		return result;

	static const char* kHexChars = "0123456789ABCDEF";
	int32 outIndex = 0;

	for (int32 i = 0; i < length; i++) {
		char c = search[i];
		if (ShouldEscape(c)) {
			buffer[outIndex++] = '%';
			buffer[outIndex++] = kHexChars[((unsigned char)c >> 4) & 0x0f];
			buffer[outIndex++] = kHexChars[(unsigned char)c & 0x0f];
		} else {
			buffer[outIndex++] = c;
		}
	}

	result.UnlockBuffer(outIndex);
	return result;
}

/*static*/ void
URLHandler::_VisitSearchEngine(const BString& search, BString& outURL, const BString& searchPageURL)
{
	BString searchQuery = search;

	// Default search URL
	BString engine(searchPageURL);

	// Check if the string starts with one of the search engine shortcuts
	for (int32 i = 0; kSearchEngines[i].url != NULL; i++) {
		int32 shortcutLen = strlen(kSearchEngines[i].shortcut);
		if (searchQuery.Compare(kSearchEngines[i].shortcut, shortcutLen) == 0) {
			if (searchQuery.Length() == shortcutLen) {
				engine = kSearchEngines[i].url;
				searchQuery.Remove(0, shortcutLen);
				break;
			} else if (searchQuery[shortcutLen] == ' ') {
				engine = kSearchEngines[i].url;
				searchQuery.Remove(0, shortcutLen + 1);
				break;
			}
		}
	}

	engine.ReplaceAll("%s", EncodeURIComponent(searchQuery));
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

		if (IsValidScheme(proto)) {
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

				// If we have a valid scheme (like ftp or others supported by WebKit
				// but not by an external app), try to load it in the browser.
				outURL = input;
				return LOAD_URL;
			}
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
		// Also handle URLs starting with "localhost" followed by a path or port.
		static const char* kLocalhostPrefix = "localhost";
		static const int32 kLocalhostLen = strlen(kLocalhostPrefix);

		if (input.Compare(kLocalhostPrefix, kLocalhostLen) == 0
			&& (input[kLocalhostLen] == '/' || input[kLocalhostLen] == ':')) {
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

			// Heuristic 1: Check if it looks like an IPv6 literal (e.g. [::1])
			if (input.Length() > 2 && input[0] == '['
				&& input.FindFirst(']') > 1) {
				bool validIPv6 = true;
				for (int32 i = 0; i < input.Length(); i++) {
					// We use IsValidDomainChar because it allows hex, colon,
					// dot (IPv4-mapped), and brackets, but disallows spaces.
					if (!IsValidDomainChar(input[i])) {
						if (input[i] == '/' || input[i] == '?'
							|| input[i] == '#') {
							break;
						}
						validIPv6 = false;
						break;
					}
				}
				if (validIPv6)
					isURL = true;
			}

			// Heuristic 2: Check if it looks like a domain name
			if (!isURL) {
				bool validChars = true;
				bool hasDot = false;

				for (int32 i = 0; i < input.Length(); i++) {
					if (input[i] == '.')
						hasDot = true;
					else if (input[i] == '/' || input[i] == '?' || input[i] == '#')
						break;
					else if (!IsValidDomainChar(input[i]) || input[i] == '[' || input[i] == ']') {
						validChars = false;
						break;
					}
				}

				if (validChars && hasDot)
					isURL = true;
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
