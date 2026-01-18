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
	while ((cookie = it.Next()) != NULL) {
		BString line;

		// domain
		line << cookie->Domain() << "\t";
		// flag (include subdomains?) - BNetworkCookie doesn't seem to expose this directly easily
		// Usually TRUE if domain starts with .
		line << (cookie->Domain().StartsWith(".") ? "TRUE" : "FALSE") << "\t";
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
	file.GetSize(&size);
	BString content;
	char* buffer = content.LockBuffer(size);
	file.Read(buffer, size);
	content.UnlockBuffer(size);

	// Parse lines
	int32 pos = 0;
	while (pos < content.Length()) {
		int32 nextPos = content.FindFirst("\n", pos);
		if (nextPos < 0) nextPos = content.Length();

		BString line;
		content.CopyInto(line, pos, nextPos - pos);
		pos = nextPos + 1;

		if (line.IsEmpty() || line.StartsWith("#"))
			continue;

		// Split by tab
		// domain flag path secure expiration name value
		// Netscape format usually has 7 fields.

		// Simple splitter
		BString parts[7];
		int32 currentPart = 0;
		int32 linePos = 0;
		while (currentPart < 7) {
			int32 tabPos = line.FindFirst("\t", linePos);
			if (tabPos < 0) {
				if (currentPart == 6) // Last part
					line.CopyInto(parts[currentPart], linePos, line.Length() - linePos);
				break;
			}
			line.CopyInto(parts[currentPart], linePos, tabPos - linePos);
			linePos = tabPos + 1;
			currentPart++;
		}

		if (currentPart >= 6) { // At least Name and Value
			BPrivate::Network::BNetworkCookie cookie(parts[5], parts[6]);
			cookie.SetDomain(parts[0]);
			cookie.SetPath(parts[2]);
			cookie.SetSecure(parts[3] == "TRUE");

			// Expiration is time_t
			time_t exp = (time_t)strtoll(parts[4].String(), NULL, 10);
			cookie.SetExpirationDate(exp);

			cookieJar.AddCookie(cookie);
		}
	}
	return B_OK;
}
