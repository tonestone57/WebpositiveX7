/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "BrowserWebView.h"

#include <InterfaceDefs.h>
#include <Window.h>


BrowserWebView::BrowserWebView(const char* name, BPrivate::Network::BUrlContext* context)
	:
	BWebView(name, context),
	fZoomTextOnly(false)
{
}


BrowserWebView::~BrowserWebView()
{
}


void
BrowserWebView::MessageReceived(BMessage* message)
{
	if (message->what == B_MOUSE_WHEEL_CHANGED) {
		// Only zoom on Command + mouse wheel
		if ((modifiers() & B_COMMAND_KEY) != 0) {
			BPoint where;
			uint32 buttons;
			GetMouse(&where, &buttons, false);

			// Only do this when the mouse is over the web view
			if (Bounds().Contains(where)) {
				float deltaY;
				if (message->FindFloat("be:wheel_delta_y", &deltaY) == B_OK) {
					if (deltaY < 0)
						IncreaseZoomFactor(fZoomTextOnly);
					else
						DecreaseZoomFactor(fZoomTextOnly);
					return;
				}
			}
		}
	}
	BWebView::MessageReceived(message);
}


void
BrowserWebView::SetZoomTextOnly(bool zoomTextOnly)
{
	fZoomTextOnly = zoomTextOnly;
}
