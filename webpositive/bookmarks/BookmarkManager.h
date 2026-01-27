/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef BOOKMARK_MANAGER_H
#define BOOKMARK_MANAGER_H

#include <Bitmap.h>
#include <Directory.h>
#include <Path.h>
#include <String.h>

class BMessage;

class BookmarkManager {
public:
	static status_t GetBookmarkPath(BPath& path);

	static void CreateBookmark(const BString& fileName, const BString& title,
		const BString& url, const BBitmap* miniIcon, const BBitmap* largeIcon);

	static void CreateBookmarkFromMessage(BMessage* message);

	static void ShowBookmarks();

	static bool CheckBookmarkExists(BDirectory& directory,
		const BString& fileName, const BString& url);

	static bool ReadURLAttr(BFile& bookmarkFile, BString& url);

	static void AddBookmarkURLsRecursively(BDirectory& directory,
		BMessage* message, uint32& addedCount);

	static status_t ImportBookmarks(const BPath& path);
	static status_t ExportBookmarks(const BPath& path);

private:
	static void _CreateBookmark(const BPath& path, BString fileName, const BString& title,
		const BString& url, const BBitmap* miniIcon, const BBitmap* largeIcon);

	static status_t _ExportBookmarksRecursively(BDirectory& directory, BFile& file,
		int32 indentLevel);
	static BString _ExtractAttribute(const BString& tag, const char* attribute);
};

#endif // BOOKMARK_MANAGER_H
