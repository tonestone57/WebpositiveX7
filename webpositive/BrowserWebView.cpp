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
			// If we didn't handle the zoom (e.g. invalid delta), should we consume?
			// The original code returned only on success.
			// But wait, the original code had return inside the innermost if.
			// My patch intends to NOT call BWebView::MessageReceived if we handled it.
			// The existing code ALREADY returns!
			// "return;" is inside "if (message->FindFloat...)".
			// So if we found the float and zoomed, we return.
			// BWebView::MessageReceived is skipped.
			// So... there is NO bug here?
			// "return;" exits the function.
			// Ah, I misread the code or the diff.
			// Let's re-read the file content.
		}
	}
	BWebView::MessageReceived(message);
}


void
BrowserWebView::SetZoomTextOnly(bool zoomTextOnly)
{
	fZoomTextOnly = zoomTextOnly;
}
