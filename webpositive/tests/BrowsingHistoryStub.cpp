/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Path.h>
#include "../BrowsingHistory.h"
#include <File.h>
#include <String.h>
#include <Message.h>

// Static member definition
BrowsingHistory BrowsingHistory::sDefaultInstance;

// Implementations
BrowsingHistory* BrowsingHistory::DefaultInstance() { return &sDefaultInstance; }
BrowsingHistory::BrowsingHistory() : BHandler(NULL), fSettingsLoaded(false), fSaveRunner(NULL), fGeneration(0) {}
BrowsingHistory::~BrowsingHistory() {}

void BrowsingHistory::MessageReceived(BMessage* message) {}
bool BrowsingHistory::AddItem(const BrowsingHistoryItem& item) { return true; }
bool BrowsingHistory::RemoveUrl(const BString& url) { return true; }
int32 BrowsingHistory::CountItems() const { return 0; }
const BrowsingHistoryItem* BrowsingHistory::HistoryItemAt(int32 index) const { return NULL; }
void BrowsingHistory::Clear() {}
void BrowsingHistory::SetMaxHistoryItemAge(int32 days) {}
int32 BrowsingHistory::MaxHistoryItemAge() const { return 0; }
void BrowsingHistory::_Clear() {}
bool BrowsingHistory::_AddItem(const BrowsingHistoryItem& item, bool invoke) { return true; }
bool BrowsingHistory::_RemoveUrl(const BString& url) { return true; }
void BrowsingHistory::_LoadSettings() {}
void BrowsingHistory::_SaveSettings(bool forceSync) {}
void BrowsingHistory::_ScheduleSave() {}
status_t BrowsingHistory::_SaveHistoryThread(void* cookie) { return B_OK; }
bool BrowsingHistory::_OpenSettingsFile(BFile& file, uint32 mode) { return true; }

// The important ones for SyncTest
status_t BrowsingHistory::ExportHistory(const BPath& path) {
    BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
    BString content = "MOCKED HISTORY";
    file.Write(content.String(), content.Length());
    return B_OK;
}

status_t BrowsingHistory::ImportHistory(const BPath& path) {
    return B_OK;
}

// BrowsingHistoryItem Stub Implementations
BrowsingHistoryItem::BrowsingHistoryItem(const BString& url) {}
BrowsingHistoryItem::BrowsingHistoryItem(const BrowsingHistoryItem& other) {}
BrowsingHistoryItem::BrowsingHistoryItem(const BMessage* archive) {}
BrowsingHistoryItem::~BrowsingHistoryItem() {}
status_t BrowsingHistoryItem::Archive(BMessage* archive) const { return B_OK; }
BrowsingHistoryItem& BrowsingHistoryItem::operator=(const BrowsingHistoryItem& other) { return *this; }
bool BrowsingHistoryItem::operator==(const BrowsingHistoryItem& other) const { return false; }
bool BrowsingHistoryItem::operator!=(const BrowsingHistoryItem& other) const { return false; }
bool BrowsingHistoryItem::operator<(const BrowsingHistoryItem& other) const { return false; }
bool BrowsingHistoryItem::operator<=(const BrowsingHistoryItem& other) const { return false; }
bool BrowsingHistoryItem::operator>(const BrowsingHistoryItem& other) const { return false; }
bool BrowsingHistoryItem::operator>=(const BrowsingHistoryItem& other) const { return false; }
void BrowsingHistoryItem::Invoked() {}
