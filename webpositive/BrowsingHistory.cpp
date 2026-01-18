/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "BrowsingHistory.h"

#include <algorithm>
#include <new>
#include <stdio.h>
#include <vector>

#include <Autolock.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Path.h>

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
	file.GetSize(&size);
	BString content;
	char* buffer = content.LockBuffer(size);
	file.Read(buffer, size);
	content.UnlockBuffer(size);

	int32 pos = 0;
	// Skip header
	int32 headerEnd = content.FindFirst("\n");
	if (headerEnd >= 0)
		pos = headerEnd + 1;

	while (pos < content.Length()) {
		int32 nextPos = pos;
		bool inQuotes = false;
		// Find end of line, respecting quotes
		while (nextPos < content.Length()) {
			char c = content.ByteAt(nextPos);
			if (c == '"') {
				inQuotes = !inQuotes;
			} else if (c == '\n' && !inQuotes) {
				break;
			}
			nextPos++;
		}

		BString line;
		content.CopyInto(line, pos, nextPos - pos);
		pos = nextPos + 1;

		if (line.IsEmpty())
			continue;

		// Parse line
		// URL,Date,Count
		// URL can be quoted.

		BString url;
		BString dateStr;
		BString countStr;

		int32 linePos = 0;
		// URL
		if (line.ByteAt(0) == '"') {
			// Quoted
			linePos = 1;
			while (linePos < line.Length()) {
				int32 quote = line.FindFirst('"', linePos);
				if (quote < 0) break; // Error
				if (quote + 1 < line.Length() && line.ByteAt(quote + 1) == '"') {
					// Escaped quote: append everything up to and including the first "
					url.Append(line.String() + linePos, quote - linePos + 1);
					// Skip the second "
					linePos = quote + 2;
				} else {
					// End of field
					url.Append(line.String() + linePos, quote - linePos);
					linePos = quote + 1;
					// Consume comma
					if (linePos < line.Length() && line.ByteAt(linePos) == ',')
						linePos++;
					break;
				}
			}
		} else {
			int32 comma = line.FindFirst(',', linePos);
			if (comma < 0) {
				// Should not happen as we expect 3 fields
				url = line;
				linePos = line.Length();
			} else {
				line.CopyInto(url, linePos, comma - linePos);
				linePos = comma + 1;
			}
		}

		// Date
		int32 comma = line.FindFirst(',', linePos);
		if (comma < 0) continue; // Invalid
		line.CopyInto(dateStr, linePos, comma - linePos);
		linePos = comma + 1;

		// Count
		line.CopyInto(countStr, linePos, line.Length() - linePos);

		struct tm timeinfo;
		memset(&timeinfo, 0, sizeof(struct tm));
		strptime(dateStr.String(), "%Y-%m-%d %H:%M:%S", &timeinfo);
		time_t t = mktime(&timeinfo);
		BDateTime dateTime;
		dateTime.SetTime_t(t);

		uint32 count = (uint32)atoi(countStr.String());

		BrowsingHistoryItem item(url);
		item.SetDateTime(dateTime);
		item.SetInvokationCount(count);

		DefaultInstance()->AddItem(item);
	}
	return B_OK;
}


// #pragma mark - BrowsingHistory


BrowsingHistory
BrowsingHistory::sDefaultInstance;
static BLocker* sSaveLock = new BLocker("history save lock");
static bool sIsShuttingDown = false;


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
	_SaveSettings(true);
	_Clear();
	delete fSaveRunner;
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


bool
BrowsingHistory::RemoveUrls(const std::vector<BString>& urls)
{
	BAutolock _(this);
	return _RemoveUrls(urls);
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

	BrowsingHistoryItem* newItem = new(std::nothrow) BrowsingHistoryItem(item);
	if (!newItem)
		return false;

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


struct PointerInList {
	PointerInList(const std::vector<BrowsingHistoryItem*>& list)
		: fList(list)
	{
	}

	bool operator()(BrowsingHistoryItem* item) const
	{
		return std::binary_search(fList.begin(), fList.end(), item);
	}

	const std::vector<BrowsingHistoryItem*>& fList;
};


bool
BrowsingHistory::_RemoveUrls(const std::vector<BString>& urls)
{
	if (urls.empty())
		return false;

	std::vector<BrowsingHistoryItem*> itemsToRemove;
	itemsToRemove.reserve(urls.size());
	bool changed = false;

	// 1. Identify items to remove using the Map (O(K * log N))
	for (size_t i = 0; i < urls.size(); i++) {
		std::map<BString, BrowsingHistoryItem*>::iterator it
			= fHistoryMap.find(urls[i]);
		if (it != fHistoryMap.end()) {
			itemsToRemove.push_back(it->second);
			fHistoryMap.erase(it);
			changed = true;
		}
	}

	if (!changed)
		return false;

	// Sort pointers for binary search (O(K log K))
	std::sort(itemsToRemove.begin(), itemsToRemove.end());

	// 2. Remove from List using remove_if (O(N * log K) with binary search)
	HistoryList::iterator newEnd = std::remove_if(fHistoryList.begin(),
		fHistoryList.end(), PointerInList(itemsToRemove));

	fHistoryList.erase(newEnd, fHistoryList.end());

	// 3. Delete items
	for (size_t i = 0; i < itemsToRemove.size(); i++) {
		delete itemsToRemove[i];
	}

	_ScheduleSave();
	fGeneration++;

	return true;
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
					BrowsingHistoryItem* newItem = new(std::nothrow) BrowsingHistoryItem(item);
					if (newItem) {
						fHistoryList.push_back(newItem);
						fHistoryMap[newItem->URL()] = newItem;
					}
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

		BAutolock _(sSaveLock);
		sIsShuttingDown = true;
		_SaveToDisk(&settingsArchive);
		return;
	}

	SaveContext* context = new(std::nothrow) SaveContext();
	if (context == NULL)
		return;

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

	thread_id thread = spawn_thread(_SaveHistoryThread,
		"save history", B_LOW_PRIORITY, context);
	if (thread >= 0) {
		resume_thread(thread);
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

	BAutolock _(sSaveLock);

	if (sIsShuttingDown) {
		delete context;
		return B_OK;
	}

	_SaveToDisk(&settingsArchive);
	delete context;
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
	fSaveRunner = new(std::nothrow) BMessageRunner(BMessenger(this),
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
