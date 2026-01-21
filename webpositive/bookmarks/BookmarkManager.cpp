/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "BookmarkManager.h"

#include <Alert.h>
#include <Catalog.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <NodeInfo.h>
#include <Roster.h>
#include <stdio.h>

#include <vector>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "WebPositive Window"

// This string is needed for the translation context.
// In the original code it was "WebPositive Window".
// We might need to ensure the catalog is loaded correctly or use B_TRANSLATE_CONTEXT.

static const char* kApplicationName = "WebPositive";

/*static*/ status_t
BookmarkManager::GetBookmarkPath(BPath& path)
{
	status_t ret = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (ret != B_OK)
		return ret;

	ret = path.Append(kApplicationName);
	if (ret != B_OK)
		return ret;

	ret = path.Append("Bookmarks");
	if (ret != B_OK)
		return ret;

	return create_directory(path.Path(), 0777);
}

/*static*/ void
BookmarkManager::CreateBookmark(const BString& fileName, const BString& title,
	const BString& url, const BBitmap* miniIcon, const BBitmap* largeIcon)
{
	BPath path;
	status_t status = GetBookmarkPath(path);

	if (status == B_OK)
		_CreateBookmark(path, fileName, title, url, miniIcon, largeIcon);
	else {
		BString message(B_TRANSLATE_COMMENT("There was an error retrieving "
			"the bookmark folder.\n\nError: %error", "Don't translate the "
			"variable %error"));
		message.ReplaceFirst("%error", strerror(status));
		BAlert* alert = new BAlert(B_TRANSLATE("Bookmark error"),
			message.String(), B_TRANSLATE("OK"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
	}
}

/*static*/ void
BookmarkManager::_CreateBookmark(const BPath& path, BString fileName, const BString& title,
	const BString& url, const BBitmap* miniIcon, const BBitmap* largeIcon)
{
	// Determine the file name if one was not provided
	bool presetFileName = true;
	if (fileName.IsEmpty() == true) {
		presetFileName = false;
		fileName = title;
		if (fileName.Length() == 0) {
			fileName = url;
			int32 leafPos = fileName.FindLast('/');
			if (leafPos >= 0)
				fileName.Remove(0, leafPos + 1);
		}
		fileName.ReplaceAll('/', '-');
		fileName.Truncate(B_FILE_NAME_LENGTH - 1);
	}

	BPath entryPath(path);
	status_t status = entryPath.Append(fileName);
	BEntry entry;
	if (status == B_OK)
		status = entry.SetTo(entryPath.Path(), true);

	// There are several reasons why an entry matching the path argument could already exist.
	if (status == B_OK && entry.Exists() == true) {
		off_t size;
		entry.GetSize(&size);
		char attrName[B_ATTR_NAME_LENGTH];
		BNode node(&entry);
		status_t attrStatus = node.GetNextAttrName(attrName);
		if (strcmp(attrName, "_trk/pinfo_le") == 0)
			attrStatus = node.GetNextAttrName(attrName);

		if (presetFileName == true && size == 0 && attrStatus == B_ENTRY_NOT_FOUND) {
			// Tracker's drag-and-drop routine created an empty entry for us to fill in.
			// Go ahead and write to the existing entry.
		} else {
			BDirectory directory(path.Path());
			if (CheckBookmarkExists(directory, fileName, url) == true) {
				// The existing entry is a bookmark with the same URL.  No further action needed.
				return;
			} else {
				// Find a unique name for the bookmark.
				int32 tries = 1;
				BString baseName = fileName;
				while (entry.Exists()) {
					fileName = baseName;
					fileName << " " << tries++;
					entryPath = path;
					status = entryPath.Append(fileName);
					if (status == B_OK)
						status = entry.SetTo(entryPath.Path(), true);
					if (status != B_OK)
						break;
				}
			}
		}
	}

	BFile bookmarkFile;
	if (status == B_OK) {
		status = bookmarkFile.SetTo(&entry,
			B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	}

	// Write bookmark meta data
	if (status == B_OK)
		status = bookmarkFile.WriteAttrString("META:url", &url);
	if (status == B_OK) {
		bookmarkFile.WriteAttrString("META:title", &title);
	}

	BNodeInfo nodeInfo(&bookmarkFile);
	if (status == B_OK) {
		status = nodeInfo.SetType("application/x-vnd.Be-bookmark");
		// Replace the standard Be-bookmark file icons with the argument icons,
		// if any were provided.
		if (status == B_OK) {
			status_t ret = B_OK;
			if (miniIcon != NULL) {
				ret = nodeInfo.SetIcon(miniIcon, B_MINI_ICON);
				if (ret != B_OK) {
					fprintf(stderr, "Failed to store mini icon for bookmark: "
						"%s\n", strerror(ret));
				}
			}
			if (largeIcon != NULL && ret == B_OK)
				ret = nodeInfo.SetIcon(largeIcon, B_LARGE_ICON);
			else if (largeIcon == NULL && miniIcon != NULL && ret == B_OK) {
				// If largeIcon is not available but miniIcon is, use a magnified miniIcon instead.
				BBitmap substituteLargeIcon(BRect(0, 0, 31, 31), B_BITMAP_NO_SERVER_LINK,
					B_CMAP8);
				const uint8* src = (const uint8*)miniIcon->Bits();
				uint32 srcBPR = miniIcon->BytesPerRow();
				uint8* dst = (uint8*)substituteLargeIcon.Bits();
				uint32 dstBPR = substituteLargeIcon.BytesPerRow();
				for (uint32 y = 0; y < 16; y++) {
					const uint8* s = src;
					uint8* d = dst;
					for (uint32 x = 0; x < 16; x++) {
						*d++ = *s;
						*d++ = *s++;
					}
					dst += dstBPR;
					s = src;
					for (uint32 x = 0; x < 16; x++) {
						*d++ = *s;
						*d++ = *s++;
					}
					dst += dstBPR;
					src += srcBPR;
				}
				ret = nodeInfo.SetIcon(&substituteLargeIcon, B_LARGE_ICON);
			} else
				ret = B_OK;
			if (ret != B_OK) {
				fprintf(stderr, "Failed to store large icon for bookmark: "
					"%s\n", strerror(ret));
			}
		}
	}

	if (status != B_OK) {
		BString message(B_TRANSLATE_COMMENT("There was an error creating the "
			"bookmark file.\n\nError: %error", "Don't translate variable "
			"%error"));
		message.ReplaceFirst("%error", strerror(status));
		BAlert* alert = new BAlert(B_TRANSLATE("Bookmark error"),
			message.String(), B_TRANSLATE("OK"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		return;
	}
}

/*static*/ void
BookmarkManager::CreateBookmarkFromMessage(BMessage* message)
{
	entry_ref ref;
	BMessage originatorData;
	const char* url;
	const char* title;
	bool validData = (message->FindRef("directory", &ref) == B_OK
		&& message->FindMessage("be:originator-data", &originatorData) == B_OK
		&& originatorData.FindString("url", &url) == B_OK
		&& originatorData.FindString("title", &title) == B_OK);

	// Optional data
	const char* fileName;
	if (message->FindString("name", &fileName) != B_OK) {
		// This string is only present if the message originated from Tracker (drag and drop).
		fileName = "";
	}
	const BBitmap* miniIcon = NULL;
	originatorData.FindData("miniIcon", B_COLOR_8_BIT_TYPE,
		reinterpret_cast<const void**>(&miniIcon), NULL);

	BBitmap* largeIcon = NULL;
	const void* largeIconData = NULL;
	ssize_t largeIconSize = 0;
	if (originatorData.FindData("largeIcon", B_RGBA32_TYPE,
			&largeIconData, &largeIconSize) == B_OK) {
		largeIcon = new BBitmap(BRect(0, 0, 31, 31),
			B_RGBA32);
		if (largeIcon->InitCheck() == B_OK
			&& largeIcon->BitsLength() == largeIconSize) {
			// Use new ImportBits signature
			largeIcon->ImportBits(largeIconData, largeIconSize, 0, 0,
				B_RGBA32);
		} else {
			delete largeIcon;
			largeIcon = NULL;
		}
	}

	if (validData == true) {
		_CreateBookmark(BPath(&ref), BString(fileName), BString(title), BString(url),
			miniIcon, largeIcon);
	} else {
		BString message(B_TRANSLATE("There was an error setting up "
			"the bookmark."));
		BAlert* alert = new BAlert(B_TRANSLATE("Bookmark error"),
			message.String(), B_TRANSLATE("OK"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
	}
	delete largeIcon;
}

/*static*/ void
BookmarkManager::ShowBookmarks()
{
	BPath path;
	entry_ref ref;
	status_t status = GetBookmarkPath(path);
	if (status == B_OK)
		status = get_ref_for_path(path.Path(), &ref);
	if (status == B_OK)
		status = be_roster->Launch(&ref);

	if (status != B_OK && status != B_ALREADY_RUNNING) {
		BString message(B_TRANSLATE_COMMENT("There was an error trying to "
			"show the Bookmarks folder.\n\nError: %error",
			"Don't translate variable %error"));
		message.ReplaceFirst("%error", strerror(status));
		BAlert* alert = new BAlert(B_TRANSLATE("Bookmark error"),
			message.String(), B_TRANSLATE("OK"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		return;
	}
}

/*static*/ bool
BookmarkManager::CheckBookmarkExists(BDirectory& directory,
	const BString& bookmarkName, const BString& url)
{
	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		char entryName[B_FILE_NAME_LENGTH];
		if (entry.GetName(entryName) != B_OK || bookmarkName != entryName)
			continue;
		BString storedURL;
		BFile file(&entry, B_READ_ONLY);
		if (ReadURLAttr(file, storedURL)) {
			// Just bail if the bookmark already exists
			if (storedURL == url)
				return true;
		}
	}
	return false;
}

/*static*/ bool
BookmarkManager::ReadURLAttr(BFile& bookmarkFile, BString& url)
{
	return bookmarkFile.InitCheck() == B_OK
		&& bookmarkFile.ReadAttrString("META:url", &url) == B_OK;
}

/*static*/ void
BookmarkManager::AddBookmarkURLsRecursively(BDirectory& directory,
	BMessage* message, uint32& addedCount)
{
	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		if (entry.IsDirectory()) {
			BDirectory subBirectory(&entry);
			// At least preserve the entry file handle when recursing into
			// sub-folders... eventually we will run out, though, with very
			// deep hierarchy.
			entry.Unset();
			AddBookmarkURLsRecursively(subBirectory, message, addedCount);
		} else {
			BString storedURL;
			BFile file(&entry, B_READ_ONLY);
			if (ReadURLAttr(file, storedURL)) {
				message->AddString("url", storedURL.String());
				addedCount++;
			}
		}
	}
}

/*static*/ status_t
BookmarkManager::ExportBookmarks(const BPath& path)
{
	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	BString header = "<!DOCTYPE NETSCAPE-Bookmark-file-1>\n"
		"<!-- This is an automatically generated file.\n"
		"     It will be read and overwritten.\n"
		"     DO NOT EDIT! -->\n"
		"<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=UTF-8\">\n"
		"<TITLE>Bookmarks</TITLE>\n"
		"<H1>Bookmarks</H1>\n"
		"<DL><p>\n";
	file.Write(header.String(), header.Length());

	BPath bookmarkPath;
	status = GetBookmarkPath(bookmarkPath);
	if (status != B_OK)
		return status;

	BDirectory dir(bookmarkPath.Path());
	_ExportBookmarksRecursively(dir, file, 1);

	BString footer = "</DL><p>\n";
	file.Write(footer.String(), footer.Length());
	return B_OK;
}

/*static*/ status_t
BookmarkManager::_ExportBookmarksRecursively(BDirectory& directory, BFile& file,
	int32 indentLevel)
{
	BEntry entry;
	directory.Rewind();
	while (directory.GetNextEntry(&entry) == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		if (entry.GetName(name) != B_OK)
			continue;

		BString indent;
		for (int32 i = 0; i < indentLevel; i++)
			indent << "    ";

		if (entry.IsDirectory()) {
			BString folderName(name);
			folderName.ReplaceAll("&", "&amp;");
			folderName.ReplaceAll("<", "&lt;");
			folderName.ReplaceAll(">", "&gt;");

			BString line;
			line << indent << "<DT><H3>" << folderName << "</H3>\n";
			line << indent << "<DL><p>\n";
			file.Write(line.String(), line.Length());

			BDirectory subDirectory(&entry);
			_ExportBookmarksRecursively(subDirectory, file, indentLevel + 1);

			line = "";
			line << indent << "</DL><p>\n";
			file.Write(line.String(), line.Length());
		} else {
			BFile bookmarkFile(&entry, B_READ_ONLY);
			BString url;
			if (ReadURLAttr(bookmarkFile, url)) {
				BString title;
				if (bookmarkFile.ReadAttrString("META:title", &title) != B_OK)
					title = name;

				title.ReplaceAll("&", "&amp;");
				title.ReplaceAll("<", "&lt;");
				title.ReplaceAll(">", "&gt;");

				BString line;
				line << indent << "<DT><A HREF=\"" << url << "\">" << title << "</A>\n";
				file.Write(line.String(), line.Length());
			}
		}
	}
	return B_OK;
}

/*static*/ status_t
BookmarkManager::ImportBookmarks(const BPath& path)
{
	BFile file(path.Path(), B_READ_ONLY);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	off_t size;
	status = file.GetSize(&size);
	if (status != B_OK)
		return status;

	// Check against reasonable limit (64MB) and int32 overflow
	if (size < 0 || size > 0x4000000)
		return B_ERROR;

	// Use malloc because BString::Adopt takes ownership and uses free()
	char* buffer = (char*)malloc(size + 1);
	if (buffer == NULL)
		return B_NO_MEMORY;

	if (file.Read(buffer, size) != size) {
		free(buffer);
		return B_IO_ERROR;
	}
	buffer[size] = '\0';

	BString content;
	content.Adopt(buffer, size);

	BPath bookmarkPath;
	status = GetBookmarkPath(bookmarkPath);
	if (status != B_OK)
		return status;

	std::vector<BPath> dirStack;
	dirStack.push_back(bookmarkPath);

	BString pendingFolderName;

	int32 pos = 0;
	while (pos < content.Length()) {
		int32 tagStart = content.FindFirst("<", pos);
		if (tagStart < 0) break;
		int32 tagEnd = content.FindFirst(">", tagStart);
		if (tagEnd < 0) break;

		BString tag = content.String() + tagStart + 1;
		tag.Truncate(tagEnd - tagStart - 1);
		BString tagUpper = tag;
		tagUpper.ToUpper();

		if (tagUpper.StartsWith("DT")) {
			// Found DT, look for H3 or A in next tag
			int32 nextTagStart = content.FindFirst("<", tagEnd);
			if (nextTagStart > 0) {
				int32 nextTagEnd = content.FindFirst(">", nextTagStart);
				if (nextTagEnd > 0) {
					BString nextTag = content.String() + nextTagStart + 1;
					nextTag.Truncate(nextTagEnd - nextTagStart - 1);
					BString nextTagUpper = nextTag;
					nextTagUpper.ToUpper();

					if (nextTagUpper.StartsWith("H3")) {
						// Folder name
						BString endTag = "</H3>";
						// Note: This is case sensitive search, but should be fine for now
						// or we can use IFindFirst if available (not in Haiku public API?)
						// We'll search for </H3> or </h3>
						int32 closeH3 = content.FindFirst(endTag, nextTagEnd);
						if (closeH3 < 0) closeH3 = content.FindFirst("</h3>", nextTagEnd);

						if (closeH3 > 0) {
							BString folderName;
							content.CopyInto(folderName, nextTagEnd + 1, closeH3 - nextTagEnd - 1);

							folderName.ReplaceAll("&lt;", "<");
							folderName.ReplaceAll("&gt;", ">");
							folderName.ReplaceAll("&amp;", "&");

							pendingFolderName = folderName;
							pos = closeH3 + 5;
							continue;
						}
					} else if (nextTagUpper.StartsWith("A")) {
						// Bookmark
						BString tagContent = nextTag;
						int32 hrefPos = tagContent.IFindFirst("HREF=\"");
						if (hrefPos >= 0) {
							int32 hrefEnd = tagContent.FindFirst("\"", hrefPos + 6);
							if (hrefEnd > 0) {
								BString url;
								tagContent.CopyInto(url, hrefPos + 6, hrefEnd - hrefPos - 6);

								int32 closeA = content.FindFirst("</A>", nextTagEnd);
								if (closeA < 0) closeA = content.FindFirst("</a>", nextTagEnd);

								if (closeA > 0) {
									BString title;
									content.CopyInto(title, nextTagEnd + 1, closeA - nextTagEnd - 1);

									title.ReplaceAll("&lt;", "<");
									title.ReplaceAll("&gt;", ">");
									title.ReplaceAll("&amp;", "&");

									_CreateBookmark(dirStack.back(), title, title, url, NULL, NULL);
									pos = closeA + 4;
									continue;
								}
							}
						}
					}
				}
			}
		} else if (tagUpper.StartsWith("DL")) {
			if (!pendingFolderName.IsEmpty()) {
				BPath currentPath = dirStack.back();
				currentPath.Append(pendingFolderName);
				create_directory(currentPath.Path(), 0777);
				dirStack.push_back(currentPath);
				pendingFolderName = "";
			}
		} else if (tagUpper.StartsWith("/DL")) {
			if (dirStack.size() > 1)
				dirStack.pop_back();
		}

		pos = tagEnd + 1;
	}

	return B_OK;
}
