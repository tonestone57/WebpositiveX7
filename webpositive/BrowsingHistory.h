/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef BROWSING_HISTORY_H
#define BROWSING_HISTORY_H

#include "DateTime.h"
#include <Handler.h>
#include <List.h>
#include <Locker.h>
#include <String.h>

#include <map>

class BFile;
class BMessageRunner;


class BrowsingHistoryItem {
public:
								BrowsingHistoryItem(const BString& url);
								BrowsingHistoryItem(
									const BrowsingHistoryItem& other);
								BrowsingHistoryItem(const BMessage* archive);
								~BrowsingHistoryItem();

			status_t			Archive(BMessage* archive) const;

			BrowsingHistoryItem& operator=(const BrowsingHistoryItem& other);

			bool				operator==(
									const BrowsingHistoryItem& other) const;
			bool				operator!=(
									const BrowsingHistoryItem& other) const;
			bool				operator<(
									const BrowsingHistoryItem& other) const;
			bool				operator<=(
									const BrowsingHistoryItem& other) const;
			bool				operator>(
									const BrowsingHistoryItem& other) const;
			bool				operator>=(
									const BrowsingHistoryItem& other) const;

			const BString&		URL() const { return fURL; }
			const BDateTime&	DateTime() const { return fDateTime; }
			uint32				InvokationCount() const {
									return fInvokationCount; }
			void				Invoked();

private:
			BString				fURL;
			BDateTime			fDateTime;
			uint32				fInvokationCount;
};


class BrowsingHistory : public BHandler, public BLocker {
public:
	static	BrowsingHistory*	DefaultInstance();

	virtual	void				MessageReceived(BMessage* message);

			bool				AddItem(const BrowsingHistoryItem& item);
			bool				RemoveUrl(const BString& url);

	// Should Lock() the object when using these in some loop or so:
			int32				CountItems() const;
			const BrowsingHistoryItem*	HistoryItemAt(int32 index) const;
			void				Clear();

			void				SetMaxHistoryItemAge(int32 days);
			int32				MaxHistoryItemAge() const;

private:
								BrowsingHistory();
	virtual						~BrowsingHistory();

			void				_Clear();
			bool				_AddItem(const BrowsingHistoryItem& item,
									bool invoke);
			bool				_RemoveUrl(const BString& url);

			void				_LoadSettings();
			void				_SaveSettings(bool forceSync = false);
			void				_ScheduleSave();
	static	status_t			_SaveHistoryThread(void* cookie);
			bool				_OpenSettingsFile(BFile& file, uint32 mode);

private:
			BList				fHistoryItems;
			std::map<BString, BrowsingHistoryItem*> fHistoryMap;
			int32				fMaxHistoryItemAge;

	static	BrowsingHistory		sDefaultInstance;
			bool				fSettingsLoaded;
			BMessageRunner*		fSaveRunner;
};


#endif // BROWSING_HISTORY_H

