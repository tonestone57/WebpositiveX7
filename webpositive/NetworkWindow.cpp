/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "NetworkWindow.h"

#include <Button.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <ScrollView.h>
#include <StringItem.h>

#include "BrowserApp.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Network Window"

NetworkWindow::NetworkWindow(BRect frame)
	:
	BWindow(frame, B_TRANSLATE("Network Inspector (Page Loads)"), B_TITLED_WINDOW,
		B_NORMAL_WINDOW_FEEL, B_AUTO_UPDATE_SIZE_LIMITS
			| B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE)
{
	SetLayout(new BGroupLayout(B_VERTICAL, 0.0));

	fRequestListView = new BListView("Network requests");

	fClearButton = new BButton(B_TRANSLATE("Clear"),
		new BMessage(CLEAR_NETWORK_REQUESTS));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0.0)
		.Add(new BScrollView("Network requests scroll",
			fRequestListView, 0, true, true))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.AddGlue()
			.Add(fClearButton)
			.AddGlue()
			.SetInsets(0, B_USE_SMALL_SPACING, 0, 0))
		.SetInsets(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING,
			B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
	);

	if (!frame.IsValid())
		CenterOnScreen();
}

NetworkWindow::~NetworkWindow()
{
	int32 count = fRequestListView->CountItems();
	for (int32 i = count - 1; i >= 0; i--)
		delete fRequestListView->RemoveItem(i);
}

void
NetworkWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case ADD_NETWORK_REQUEST:
		{
			BString url;
			if (message->FindString("url", &url) == B_OK) {
				BString displayText;
				displayText.SetToFormat("[Pending] %s (Headers: N/A)", url.String());
				fRequestListView->AddItem(new BStringItem(displayText));
				fRequestListView->ScrollTo(fRequestListView->CountItems() - 1);
			}
			break;
		}
		case UPDATE_NETWORK_REQUEST:
		{
			BString url;
			BString status;
			if (message->FindString("url", &url) == B_OK &&
				message->FindString("status", &status) == B_OK) {

				// Construct the expected "Pending" string to match exactly
				BString pendingText;
				pendingText.SetToFormat("[Pending] %s (Headers: N/A)", url.String());

				// Find item with exact pending text and update it
				for (int32 i = fRequestListView->CountItems() - 1; i >= 0; i--) {
					BStringItem* item = (BStringItem*)fRequestListView->ItemAt(i);
					if (item->Text() == pendingText) {
						BString displayText;
						displayText.SetToFormat("[%s] %s (Headers: N/A)", status.String(), url.String());
						item->SetText(displayText);
						fRequestListView->InvalidateItem(i);
						break; // Assume last one is the relevant one
					}
				}
			}
			break;
		}
		case CLEAR_NETWORK_REQUESTS:
		{
			int32 count = fRequestListView->CountItems();
			for (int32 i = count - 1; i >= 0; i--)
				delete fRequestListView->RemoveItem(i);
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}

bool
NetworkWindow::QuitRequested()
{
	if (!IsHidden())
		Hide();
	return false;
}
