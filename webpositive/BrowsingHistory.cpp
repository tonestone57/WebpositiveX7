/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "BrowsingHistory.h"

#include <new>
#include <stdio.h>

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


// #pragma mark - BrowsingHistory


BrowsingHistory
BrowsingHistory::sDefaultInstance;
static BLocker* sSaveLock = new BLocker("history save lock");
static bool sIsShuttingDown = false;


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
	fHistoryItems(64),
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


int32
BrowsingHistory::BrowsingHistory::CountItems() const
{
	BAutolock _(const_cast<BrowsingHistory*>(this));

	return fHistoryItems.CountItems();
}


const BrowsingHistoryItem*
BrowsingHistory::HistoryItemAt(int32 index) const
{
	BAutolock _(const_cast<BrowsingHistory*>(this));

	return reinterpret_cast<BrowsingHistoryItem*>(fHistoryItems.ItemAt(index));
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
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		BrowsingHistoryItem* item = reinterpret_cast<BrowsingHistoryItem*>(
			fHistoryItems.ItemAtFast(i));
		delete item;
	}
	fHistoryItems.MakeEmpty();
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
			it->second->Invoked();
			_ScheduleSave();
		}
		return true;
	}

	int32 count = CountItems();
	int32 insertionIndex = count;

	// Binary search for insertion index (O(log N))
	int32 low = 0;
	int32 high = count - 1;
	while (low <= high) {
		int32 mid = low + (high - low) / 2;
		const BrowsingHistoryItem* midItem
			= (const BrowsingHistoryItem*)fHistoryItems.ItemAtFast(mid);

		if (item < *midItem) {
			insertionIndex = mid;
			high = mid - 1;
		} else {
			low = mid + 1;
		}
	}
	BrowsingHistoryItem* newItem = new(std::nothrow) BrowsingHistoryItem(item);
	if (!newItem || !fHistoryItems.AddItem(newItem, insertionIndex)) {
		delete newItem;
		return false;
	}

	try {
		fHistoryMap[newItem->URL()] = newItem;
	} catch (...) {
		fHistoryItems.RemoveItem(newItem);
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


bool
BrowsingHistory::_RemoveUrl(const BString& url)
{
	std::map<BString, BrowsingHistoryItem*>::iterator it
		= fHistoryMap.find(url);
	if (it == fHistoryMap.end())
		return false;

	BrowsingHistoryItem* item = it->second;
	fHistoryItems.RemoveItem(item);
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
		for (int32 i = 0; settingsArchive.FindMessage("history item", i,
				&historyItemArchive) == B_OK; i++) {
			BrowsingHistoryItem item(&historyItemArchive);
			if (oldestAllowedDateTime < item.DateTime())
				_AddItem(item, true);
			historyItemArchive.MakeEmpty();
		}
	}
}


void
BrowsingHistory::_SaveSettings(bool forceSync)
{
	BMessage* settingsArchive = new(std::nothrow) BMessage();
	if (settingsArchive == NULL)
		return;

	settingsArchive->AddInt32("max history item age", fMaxHistoryItemAge);
	BMessage historyItemArchive;
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		const BrowsingHistoryItem* item = HistoryItemAt(i);
		if (!item || item->Archive(&historyItemArchive) != B_OK)
			break;
		if (settingsArchive->AddMessage("history item",
				&historyItemArchive) != B_OK) {
			break;
		}
		historyItemArchive.MakeEmpty();
	}

	if (forceSync) {
		BAutolock _(sSaveLock);
		sIsShuttingDown = true;
		_SaveToDisk(settingsArchive);
		delete settingsArchive;
	} else {
		thread_id thread = spawn_thread(_SaveHistoryThread,
			"save history", B_LOW_PRIORITY, settingsArchive);
		if (thread >= 0) {
			resume_thread(thread);
		} else {
			_SaveHistoryThread(settingsArchive);
		}
	}
}


/*static*/ status_t
BrowsingHistory::_SaveHistoryThread(void* cookie)
{
	BMessage* settingsArchive = (BMessage*)cookie;
	BAutolock _(sSaveLock);

	if (sIsShuttingDown) {
		delete settingsArchive;
		return B_OK;
	}

	_SaveToDisk(settingsArchive);
	delete settingsArchive;
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

