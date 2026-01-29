/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NETWORK_WINDOW_H
#define NETWORK_WINDOW_H

#include <Window.h>
#include <Messenger.h>
#include <String.h>
#include <StringItem.h>

#include <deque>
#include <map>

class BListView;
class BButton;

class NetworkRequestItem : public BStringItem {
public:
	NetworkRequestItem(const char* text, const char* url)
		:
		BStringItem(text),
		fUrl(url),
		fIsPending(true)
	{
	}

	const BString& Url() const
	{
		return fUrl;
	}

	bool IsPending() const
	{
		return fIsPending;
	}

	void SetPending(bool pending)
	{
		fIsPending = pending;
	}

private:
	BString fUrl;
	bool fIsPending;
};

enum {
	ADD_NETWORK_REQUEST = 'anrq',
	UPDATE_NETWORK_REQUEST = 'unrq',
	CLEAR_NETWORK_REQUESTS = 'cnrq'
};

class NetworkWindow : public BWindow {
public:
	NetworkWindow(BRect frame);
	virtual ~NetworkWindow();
	virtual void MessageReceived(BMessage* message);
	virtual bool QuitRequested();
	virtual void Show();

	void SetTarget(const BMessenger& target);
	void PrepareToQuit();

private:
	void _AppendRequest(NetworkRequestItem* item);
	void _UpdateList();

private:
	BListView* fRequestListView;
	BButton* fClearButton;
	std::map<BString, std::deque<NetworkRequestItem*> > fPendingRequests;
	std::deque<NetworkRequestItem*> fAllRequests;
	BMessenger fTarget;
	bool fQuitting;
};

#endif // NETWORK_WINDOW_H
