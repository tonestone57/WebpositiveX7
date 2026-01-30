/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "Sync.h"

#include "BookmarkManager.h"
#include "BrowsingHistory.h"

#include <Directory.h>
#include <File.h>
#include <NetworkCookie.h>
#include <NetworkCookieJar.h>
#include <String.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>

/*static*/ status_t
Sync::ExportProfile(const BPath& folder,
	BPrivate::Network::BNetworkCookieJar& cookieJar)
{
	status_t status = create_directory(folder.Path(), 0777);
	if (status != B_OK)
		return status;

	BPath bookmarksPath(folder);
	bookmarksPath.Append("bookmarks.html");
	status = BookmarkManager::ExportBookmarks(bookmarksPath);
	if (status != B_OK) return status;

	BPath historyPath(folder);
	historyPath.Append("history.csv");
	status = BrowsingHistory::ExportHistory(historyPath);
	if (status != B_OK) return status;

	BPath cookiesPath(folder);
	cookiesPath.Append("cookies.txt");
	status = ExportCookies(cookiesPath, cookieJar);
	if (status != B_OK) return status;

	return B_OK;
}

/*static*/ status_t
Sync::ImportProfile(const BPath& folder,
	BPrivate::Network::BNetworkCookieJar& cookieJar)
{
	BPath bookmarksPath(folder);
	bookmarksPath.Append("bookmarks.html");
	BookmarkManager::ImportBookmarks(bookmarksPath);
	// We ignore errors here as some files might be missing

	BPath historyPath(folder);
	historyPath.Append("history.csv");
	BrowsingHistory::ImportHistory(historyPath);

	BPath cookiesPath(folder);
	cookiesPath.Append("cookies.txt");
	ImportCookies(cookiesPath, cookieJar);

	return B_OK;
}

/*static*/ status_t
Sync::ExportCookies(const BPath& path,
	BPrivate::Network::BNetworkCookieJar& cookieJar)
{
	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t status = file.InitCheck();
	if (status != B_OK) return status;

	BString header = "# Netscape HTTP Cookie File\n"
		"# http://curl.haxx.se/rfc/cookie_spec.html\n"
		"# This is a generated file!  Do not edit.\n\n";
	file.Write(header.String(), header.Length());

	BPrivate::Network::BNetworkCookieJar::Iterator it = cookieJar.GetIterator();
	const BPrivate::Network::BNetworkCookie* cookie;
	BString line;
	while ((cookie = it.Next()) != NULL) {
		line.Truncate(0);

		// domain
		if (cookie->HttpOnly())
			line << "#HttpOnly_" << cookie->Domain() << "\t";
		else
			line << cookie->Domain() << "\t";

		// flag (include subdomains?)
		line << (!cookie->IsHostOnly() ? "TRUE" : "FALSE") << "\t";
		// path
		line << cookie->Path() << "\t";
		// secure
		line << (cookie->Secure() ? "TRUE" : "FALSE") << "\t";
		// expiration
		line << cookie->ExpirationDate() << "\t";
		// name
		line << cookie->Name() << "\t";
		// value
		line << cookie->Value() << "\n";

		file.Write(line.String(), line.Length());
	}
	return B_OK;
}

/*static*/ status_t
Sync::ImportCookies(const BPath& path,
	BPrivate::Network::BNetworkCookieJar& cookieJar)
{
	BFile file(path.Path(), B_READ_ONLY);
	status_t status = file.InitCheck();
	if (status != B_OK) return status;

	off_t size;
	status = file.GetSize(&size);
	if (status != B_OK) return status;

	// Check against reasonable limit (64MB) and int32 overflow
	if (size < 0 || size > 0x4000000)
		return B_ERROR;

	// Optimize: allocate raw buffer to avoid BString allocations
	char* buffer = new(std::nothrow) char[size + 1];
	if (buffer == NULL)
		return B_NO_MEMORY;

	std::unique_ptr<char[]> bufferPtr(buffer);

	if (file.Read(buffer, size) != size)
		return B_IO_ERROR;
	buffer[size] = '\0';

	char* cursor = buffer;
	char* end = buffer + size;

	while (cursor < end && *cursor) {
		char* lineStart = cursor;
		// Find end of line
		char* lineEnd = strchr(cursor, '\n');
		if (lineEnd) {
			*lineEnd = '\0';
			cursor = lineEnd + 1;
		} else {
			// Last line
			cursor = end;
			lineEnd = end;
		}

		if (lineEnd - lineStart == 0)
			continue;

		// Check for comment
		bool httpOnly = false;
		if (*lineStart == '#') {
			if (strncmp(lineStart, "#HttpOnly_", 10) == 0) {
				httpOnly = true;
				lineStart += 10;
			} else {
				continue;
			}
		}

		// Split by tab
		// domain flag path secure expiration name value
		char* parts[7];
		int partCount = 0;
		char* token = lineStart;

		while (partCount < 7) {
			parts[partCount++] = token;
			char* tab = strchr(token, '\t');
			if (tab && tab < lineEnd) {
				*tab = '\0';
				token = tab + 1;
			} else {
				// Ensure last token is valid
				if (token >= lineEnd && partCount < 7) {
					// Missing fields
					partCount--;
				}
				break;
			}
		}

		if (partCount >= 7) {
			BPrivate::Network::BNetworkCookie cookie(parts[5], parts[6]);
			cookie.SetDomain(parts[0]);
			cookie.SetPath(parts[2]);
			cookie.SetSecure(strcmp(parts[3], "TRUE") == 0);
			cookie.SetHttpOnly(httpOnly);

			// Expiration is time_t
			time_t exp = (time_t)strtoll(parts[4], NULL, 10);
			cookie.SetExpirationDate(exp);

			cookieJar.AddCookie(cookie);
		}
	}

	// bufferPtr will delete buffer
	bufferPtr.reset();
	return B_OK;
}
