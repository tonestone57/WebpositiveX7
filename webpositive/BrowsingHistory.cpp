/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "BrowsingHistory.h"

#include <algorithm>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>

#include <Autolock.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Path.h>
#include <Url.h>

#include "BrowserApp.h"


static const uint32 SAVE_HISTORY = 'svhs';


BrowsingHistoryItem::BrowsingHistoryItem(const BString& url)
	:
	fURL(url),
	fDateTime(BDateTime::CurrentDateTime(B_LOCAL_TIME)),
	fInvokationCount(0)
{
}


BrowsingHistoryItem::BrowsingHistoryItem(const BrowsingHistoryItem& other)
{
	*this = other;
}


BrowsingHistoryItem::BrowsingHistoryItem(const BMessage* archive)
{
	if (!archive)
		return;
	BMessage dateTimeArchive;
	if (archive->FindMessage("date time", &dateTimeArchive) == B_OK)
		fDateTime = BDateTime(&dateTimeArchive);
	archive->FindString("url", &fURL);
	archive->FindUInt32("invokations", &fInvokationCount);
}


BrowsingHistoryItem::~BrowsingHistoryItem()
{
}


status_t
BrowsingHistoryItem::Archive(BMessage* archive) const
{
	if (!archive)
		return B_BAD_VALUE;
	BMessage dateTimeArchive;
	status_t status = fDateTime.Archive(&dateTimeArchive);
	if (status == B_OK)
		status = archive->AddMessage("date time", &dateTimeArchive);
	if (status == B_OK)
		status = archive->AddString("url", fURL.String());
	if (status == B_OK)
		status = archive->AddUInt32("invokations", fInvokationCount);
	return status;
}


BrowsingHistoryItem&
BrowsingHistoryItem::operator=(const BrowsingHistoryItem& other)
{
	if (this == &other)
		return *this;

	fURL = other.fURL;
	fDateTime = other.fDateTime;
	fInvokationCount = other.fInvokationCount;

	return *this;
}


bool
BrowsingHistoryItem::operator==(const BrowsingHistoryItem& other) const
{
	if (this == &other)
		return true;

	return fURL == other.fURL && fDateTime == other.fDateTime
		&& fInvokationCount == other.fInvokationCount;
}


bool
BrowsingHistoryItem::operator!=(const BrowsingHistoryItem& other) const
{
	return !(*this == other);
}


bool
BrowsingHistoryItem::operator<(const BrowsingHistoryItem& other) const
{
	if (this == &other)
		return false;

	if (fDateTime < other.fDateTime)
		return true;
	if (other.fDateTime < fDateTime)
		return false;
	return fURL < other.fURL;
}


bool
BrowsingHistoryItem::operator<=(const BrowsingHistoryItem& other) const
{
	return !(*this > other);
	return !(other < *this);
}


bool
BrowsingHistoryItem::operator>(const BrowsingHistoryItem& other) const
{
	return other < *this;
}


bool
BrowsingHistoryItem::operator>=(const BrowsingHistoryItem& other) const
{
	return !(*this < other);
}


void
BrowsingHistoryItem::Invoked()
{
	// Eventually, we may overflow...
	uint32 count = fInvokationCount + 1;
	if (count > fInvokationCount)
		fInvokationCount = count;
	fDateTime = BDateTime::CurrentDateTime(B_LOCAL_TIME);
}


/*static*/ status_t
BrowsingHistory::ExportHistory(const BPath& path)
{
	BAutolock lock(DefaultInstance());
	if (!lock.IsLocked())
		return B_ERROR;

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	BString header = "URL,Date,Count\n";
	file.Write(header.String(), header.Length());

	for (int32 i = 0; i < DefaultInstance()->CountItems(); i++) {
		const BrowsingHistoryItem* item = DefaultInstance()->HistoryItemAt(i);
		if (item) {
			BString line;
			BString url = item->URL();
			if (url.FindFirst(',') >= 0 || url.FindFirst('"') >= 0) {
				url.ReplaceAll("\"", "\"\"");
				line << "\"" << url << "\",";
			} else {
				line << url << ",";
			}

			// Format date
			char dateStr[64];
			time_t t = item->DateTime().Time_t();
			strftime(dateStr, sizeof(dateStr), "%Y-%m-%d %H:%M:%S", localtime(&t));

			line << dateStr << ",";
			line << item->InvokationCount() << "\n";

			file.Write(line.String(), line.Length());
		}
	}
	return B_OK;
}


/*static*/ status_t
BrowsingHistory::ImportHistory(const BPath& path)
{
	BFile file(path.Path(), B_READ_ONLY);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	off_t size;
	status = file.GetSize(&size);
	if (status != B_OK)
		return status;

	// 256 MB seems like a reasonable upper limit for a history file
	if (size < 0 || size > 0x10000000)
		return B_ERROR;

	// Optimize: raw buffer for manual parsing
	char* buffer = new(std::nothrow) char[size + 1];
	if (buffer == NULL)
		return B_NO_MEMORY;

	if (file.Read(buffer, size) != size) {
		delete[] buffer;
		return B_IO_ERROR;
	}
	buffer[size] = '\0';

	std::vector<BrowsingHistoryItem> items;
	// Pre-allocate to reduce reallocations
	items.reserve(size / 50);

	char* cursor = buffer;
	char* end = buffer + size;

	// Skip header
	while (cursor < end && *cursor != '\n') cursor++;
	if (cursor < end) cursor++;

	while (cursor < end && *cursor) {
		char* lineStart = cursor;
		// Find end of line, respecting quotes
		bool inQuotes = false;
		while (cursor < end) {
			if (*cursor == '"') inQuotes = !inQuotes;
			else if (*cursor == '\n' && !inQuotes) break;
			cursor++;
		}

		size_t lineLen = cursor - lineStart;
		if (lineLen == 0) {
			if (cursor < end) cursor++;
			continue;
		}

		// Temporarily null-terminate the line
		char originalChar = *cursor;
		*cursor = '\0';

		// Parse Line
		char* token = lineStart;
		BString url;

		// URL
		if (*token == '"') {
			token++; // Skip opening quote
			// Manual unescaping
			while (*token) {
				if (*token == '"') {
					if (*(token + 1) == '"') {
						url += '"';
						token += 2;
					} else {
						// End quote
						token++;
						if (*token == ',') token++;
						break;
					}
				} else {
					// Optimization: append chunk until next quote
					char* nextQuote = strchr(token, '"');
					if (nextQuote) {
						url.Append(token, nextQuote - token);
						token = nextQuote;
					} else {
						// Should not happen if well-formed
						url += token;
						break;
					}
				}
			}
		} else {
			char* comma = strchr(token, ',');
			if (comma) {
				url.Append(token, comma - token);
				token = comma + 1;
			} else {
				url = token;
				token += strlen(token);
			}
		}

		// Date
		char* comma = strchr(token, ',');
		if (comma) {
			*comma = '\0';
			struct tm timeinfo;
			memset(&timeinfo, 0, sizeof(struct tm));
			strptime(token, "%Y-%m-%d %H:%M:%S", &timeinfo);
			time_t t = mktime(&timeinfo);
			BDateTime dateTime;
			dateTime.SetTime_t(t);

			// Count
			token = comma + 1;
			uint32 count = (uint32)atoi(token);

			BrowsingHistoryItem item(url);
			item.SetDateTime(dateTime);
			item.SetInvokationCount(count);
			items.push_back(item);
		}

		// Restore and advance
		*cursor = originalChar;
		if (cursor < end) cursor++;
	}

	delete[] buffer;

	if (items.empty())
		return B_OK;

	BAutolock lock(DefaultInstance());
	if (!lock.IsLocked())
		return B_ERROR;

	int32 importedCount = 0;
	for (size_t i = 0; i < items.size(); i++) {
		const BrowsingHistoryItem& item = items[i];
		if (DefaultInstance()->fHistoryMap.find(item.URL())
				== DefaultInstance()->fHistoryMap.end()) {
			// Item does not exist, add it
			BrowsingHistoryItem* newItem = new BrowsingHistoryItem(item);
			bool pushed = false;
			try {
				DefaultInstance()->fHistoryList.push_back(newItem);
				pushed = true;
				DefaultInstance()->fHistoryMap[newItem->URL()] = newItem;
				importedCount++;
			} catch (...) {
				// In case of allocation error in the map
				if (pushed)
					DefaultInstance()->fHistoryList.pop_back();
				delete newItem;
				// Continue to next item
			}
		}
	}

	if (importedCount > 0) {
		std::sort(DefaultInstance()->fHistoryList.begin(),
			DefaultInstance()->fHistoryList.end(),
			BrowsingHistoryItemPointerCompare());
		DefaultInstance()->fGeneration++;
		DefaultInstance()->_ScheduleSave();
	}

	return B_OK;
}


// #pragma mark - BrowsingHistory


static BLocker sSaveLock("history save lock");
BrowsingHistory
BrowsingHistory::sDefaultInstance;
static bool sIsShuttingDown = false;
static int32 sActiveSaveThreads = 0;


struct SaveContext {
	std::vector<BrowsingHistoryItem> items;
	int32 maxAge;
};


static void
_SaveToDisk(BMessage* settingsArchive)
{
	// Implement logic similar to _OpenSettingsFile but standalone to avoid
	// dependency on the singleton instance which might be destroyed.
	BFile settingsFile;
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK
		&& path.Append(kApplicationName) == B_OK
		&& path.Append("BrowsingHistory") == B_OK) {
		if (settingsFile.SetTo(path.Path(), B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY) == B_OK) {
			settingsArchive->Flatten(&settingsFile);
		}
	}
}


BrowsingHistory::BrowsingHistory()
	:
	BHandler("browsing history"),
	BLocker("browsing history"),
	fMaxHistoryItemAge(7),
	fSettingsLoaded(false),
	fSaveRunner(NULL),
	fGeneration(0)
{
}


BrowsingHistory::~BrowsingHistory()
{
	delete fSaveRunner;
	_SaveSettings(true);

	while (atomic_get(&sActiveSaveThreads) > 0)
		snooze(10000);

	_Clear();
}


/*static*/ BrowsingHistory*
BrowsingHistory::DefaultInstance()
{
	if (sDefaultInstance.Lock()) {
		sDefaultInstance._LoadSettings();
		sDefaultInstance.Unlock();
	}
	return &sDefaultInstance;
}


bool
BrowsingHistory::AddItem(const BrowsingHistoryItem& item)
{
	BAutolock _(this);

	return _AddItem(item, false);
}


bool
BrowsingHistory::RemoveUrl(const BString& url)
{
	BAutolock _(this);
	return _RemoveUrl(url);
}


void
BrowsingHistory::RemoveItemsForDomain(const char* domain)
{
	BAutolock _(this);
	_RemoveItemsForDomain(domain);
}


int32
BrowsingHistory::BrowsingHistory::CountItems() const
{
	BAutolock _(const_cast<BrowsingHistory*>(this));

	return fHistoryList.size();
}


const BrowsingHistoryItem*
BrowsingHistory::HistoryItemAt(int32 index) const
{
	BAutolock _(const_cast<BrowsingHistory*>(this));
	return fHistoryList[index];
}


void
BrowsingHistory::Clear()
{
	BAutolock _(this);
	_Clear();
	_ScheduleSave();
}	


void
BrowsingHistory::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case SAVE_HISTORY:
		{
			BAutolock _(this);
			_SaveSettings(false);
			delete fSaveRunner;
			fSaveRunner = NULL;
			break;
		}
		default:
			BHandler::MessageReceived(message);
	}
}


void
BrowsingHistory::SetMaxHistoryItemAge(int32 days)
{
	BAutolock _(this);
	if (fMaxHistoryItemAge != days) {
		fMaxHistoryItemAge = days;
		_ScheduleSave();
	}
}	


int32
BrowsingHistory::MaxHistoryItemAge() const
{
	return fMaxHistoryItemAge;
}	


// #pragma mark - private


void
BrowsingHistory::_Clear()
{
	for (HistoryList::iterator it = fHistoryList.begin();
		it != fHistoryList.end(); ++it) {
		delete *it;
	}
	fHistoryList.clear();
	fHistoryMap.clear();
	fGeneration++;
}


bool
BrowsingHistory::_AddItem(const BrowsingHistoryItem& item, bool internal)
{
	std::map<BString, BrowsingHistoryItem*>::iterator it
		= fHistoryMap.find(item.URL());
	if (it != fHistoryMap.end()) {
		if (!internal) {
			// Update the item position to keep list sorted (by date/url)
			// Remove from old position
			BrowsingHistoryItem* historyItem = it->second;

			HistoryList::iterator listIt = std::lower_bound(fHistoryList.begin(),
				fHistoryList.end(), historyItem, BrowsingHistoryItemPointerCompare());

			// We might find a different item with same sort key, or the item itself.
			// Since we have the exact pointer, we can verify.
			// But std::lower_bound returns the first item that is NOT < historyItem.
			// Since historyItem is in the list, *listIt should be equal to historyItem
			// according to the comparator, but we need to find the exact pointer
			// if there are duplicates in sort order (unlikely with URL tie-break).
			// However, since we are updating Invoked(), the sort key CHANGES.
			// So we must remove it BEFORE updating.

			// Warning: lower_bound uses the current values. If we haven't changed
			// the item yet, we can find it.
			while (listIt != fHistoryList.end() && *listIt != historyItem
					&& !BrowsingHistoryItemPointerCompare()(historyItem, *listIt)) {
				++listIt;
			}

			if (listIt != fHistoryList.end() && *listIt == historyItem) {
				fHistoryList.erase(listIt);
			}

			historyItem->Invoked();

			// Re-insert
			listIt = std::lower_bound(fHistoryList.begin(),
				fHistoryList.end(), historyItem, BrowsingHistoryItemPointerCompare());
			fHistoryList.insert(listIt, historyItem);

			_ScheduleSave();
		}
		return true;
	}

	BrowsingHistoryItem* newItem = new BrowsingHistoryItem(item);

	try {
		HistoryList::iterator listIt = std::lower_bound(fHistoryList.begin(),
			fHistoryList.end(), newItem, BrowsingHistoryItemPointerCompare());
		listIt = fHistoryList.insert(listIt, newItem);
		try {
			fHistoryMap[newItem->URL()] = newItem;
		} catch (...) {
			fHistoryList.erase(listIt);
			throw;
		}
	} catch (...) {
		// If vector insert fails, we need to clean up
		delete newItem;
		return false;
	}

	if (!internal) {
		newItem->Invoked();
		_ScheduleSave();
	}

	fGeneration++;

	return true;
}


void
BrowsingHistory::_RemoveItemsForDomain(const char* domain)
{
	BString targetDomain(domain);
	int32 writeIndex = 0;
	bool changed = false;
	int32 count = (int32)fHistoryList.size();

	for (int32 i = 0; i < count; i++) {
		BrowsingHistoryItem* item = fHistoryList[i];
		bool remove = false;

		// Fast pre-filter
		if (item->URL().IFindFirst(targetDomain) >= 0) {
			BUrl url(item->URL());
			if (url.Host() == targetDomain ||
				(url.Host().EndsWith(targetDomain) &&
				 url.Host().Length() > targetDomain.Length() &&
				 url.Host()[url.Host().Length() - targetDomain.Length() - 1] == '.')) {
				remove = true;
			}
		}

		if (remove) {
			fHistoryMap.erase(item->URL());
			delete item;
			changed = true;
		} else {
			if (writeIndex != i)
				fHistoryList[writeIndex] = item;
			writeIndex++;
		}
	}

	if (changed) {
		fHistoryList.resize(writeIndex);
		fGeneration++;
		_ScheduleSave();
	}
}


bool
BrowsingHistory::_RemoveUrl(const BString& url)
{
	std::map<BString, BrowsingHistoryItem*>::iterator it
		= fHistoryMap.find(url);
	if (it == fHistoryMap.end())
		return false;

	BrowsingHistoryItem* item = it->second;

	HistoryList::iterator listIt = std::lower_bound(fHistoryList.begin(),
		fHistoryList.end(), item, BrowsingHistoryItemPointerCompare());

	while (listIt != fHistoryList.end() && *listIt != item
			&& !BrowsingHistoryItemPointerCompare()(item, *listIt)) {
		++listIt;
	}

	if (listIt != fHistoryList.end() && *listIt == item) {
		fHistoryList.erase(listIt);
	}

	fHistoryMap.erase(it);
	delete item;

	_ScheduleSave();
	fGeneration++;

	return true;
}


void
BrowsingHistory::_LoadSettings()
{
	if (fSettingsLoaded)
		return;

	fSettingsLoaded = true;

	BFile settingsFile;
	if (_OpenSettingsFile(settingsFile, B_READ_ONLY)) {
		BMessage settingsArchive;
		settingsArchive.Unflatten(&settingsFile);
		if (settingsArchive.FindInt32("max history item age",
				&fMaxHistoryItemAge) != B_OK) {
			fMaxHistoryItemAge = 7;
		}
		BDateTime oldestAllowedDateTime
			= BDateTime::CurrentDateTime(B_LOCAL_TIME);
		oldestAllowedDateTime.Date().AddDays(-fMaxHistoryItemAge);

		BMessage historyItemArchive;
		int32 count = 0;
		// Count items first to reserve vector
		type_code type;
		if (settingsArchive.GetInfo("history item", &type, &count) != B_OK)
			count = 0;

		if (count > 0)
			fHistoryList.reserve(count);

		for (int32 i = 0; settingsArchive.FindMessage("history item", i,
				&historyItemArchive) == B_OK; i++) {
			BrowsingHistoryItem item(&historyItemArchive);
			if (oldestAllowedDateTime < item.DateTime()) {
				// Bulk load: create item and push back, sort later
				if (fHistoryMap.find(item.URL()) == fHistoryMap.end()) {
					BrowsingHistoryItem* newItem = new BrowsingHistoryItem(item);
					fHistoryList.push_back(newItem);
					fHistoryMap[newItem->URL()] = newItem;
				}
			}
			historyItemArchive.MakeEmpty();
		}

		// Sort the list once after bulk insertion
		std::sort(fHistoryList.begin(), fHistoryList.end(), BrowsingHistoryItemPointerCompare());
	}
}


void
BrowsingHistory::_SaveSettings(bool forceSync)
{
	if (forceSync) {
		BMessage settingsArchive;
		settingsArchive.AddInt32("max history item age", fMaxHistoryItemAge);
		BMessage historyItemArchive;
		for (HistoryList::const_iterator it = fHistoryList.begin();
			it != fHistoryList.end(); ++it) {
			const BrowsingHistoryItem* item = *it;
			if (!item || item->Archive(&historyItemArchive) != B_OK)
				break;
			if (settingsArchive.AddMessage("history item",
					&historyItemArchive) != B_OK) {
				break;
			}
			historyItemArchive.MakeEmpty();
		}

		BAutolock _(&sSaveLock);
		sIsShuttingDown = true;
		_SaveToDisk(&settingsArchive);
		return;
	}

	SaveContext* context = new SaveContext();
	context->maxAge = fMaxHistoryItemAge;

	try {
		context->items.reserve(fHistoryList.size());
		for (HistoryList::const_iterator it = fHistoryList.begin();
			it != fHistoryList.end(); ++it) {
			const BrowsingHistoryItem* item = *it;
			if (item)
				context->items.push_back(*item);
		}
	} catch (...) {
		delete context;
		return;
	}

	atomic_add(&sActiveSaveThreads, 1);
	thread_id thread = spawn_thread(_SaveHistoryThread,
		"save history", B_LOW_PRIORITY, context);
	if (thread >= 0) {
		if (resume_thread(thread) != B_OK) {
			kill_thread(thread);
			_SaveHistoryThread(context);
		}
	} else {
		_SaveHistoryThread(context);
	}
}


/*static*/ status_t
BrowsingHistory::_SaveHistoryThread(void* cookie)
{
	SaveContext* context = (SaveContext*)cookie;

	BMessage settingsArchive;
	settingsArchive.AddInt32("max history item age", context->maxAge);
	BMessage historyItemArchive;
	for (size_t i = 0; i < context->items.size(); i++) {
		if (context->items[i].Archive(&historyItemArchive) != B_OK)
			break;
		if (settingsArchive.AddMessage("history item",
				&historyItemArchive) != B_OK) {
			break;
		}
		historyItemArchive.MakeEmpty();
	}

	BAutolock _(&sSaveLock);

	if (sIsShuttingDown) {
		delete context;
		atomic_add(&sActiveSaveThreads, -1);
		return B_OK;
	}

	_SaveToDisk(&settingsArchive);
	delete context;
	atomic_add(&sActiveSaveThreads, -1);
	return B_OK;
}




void
BrowsingHistory::_ScheduleSave()
{
	if (fSaveRunner)
		return;

	if (Looper() == NULL)
		return;

	BMessage message(SAVE_HISTORY);
	fSaveRunner = new BMessageRunner(BMessenger(this),
		&message, 2000000, 1);
}


bool
BrowsingHistory::_OpenSettingsFile(BFile& file, uint32 mode)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
		|| path.Append(kApplicationName) != B_OK
		|| path.Append("BrowsingHistory") != B_OK) {
		return false;
	}
	return file.SetTo(path.Path(), mode) == B_OK;
}
