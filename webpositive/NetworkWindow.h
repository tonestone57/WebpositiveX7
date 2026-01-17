/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NETWORK_WINDOW_H
#define NETWORK_WINDOW_H

#include <Window.h>
#include <String.h>

class BListView;
class BButton;

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

private:
	BListView* fRequestListView;
	BButton* fClearButton;
};

#endif // NETWORK_WINDOW_H
