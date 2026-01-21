/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "NetworkWindow.h"
#include "BrowserWindow.h"

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
			| B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE),
	fQuitting(false)
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
	if (fTarget.IsValid())
		fTarget.PostMessage(NETWORK_WINDOW_CLOSED);

	fPendingRequests.clear();
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
				NetworkRequestItem* item = new NetworkRequestItem(displayText, url);
				if (fRequestListView->AddItem(item)) {
					try {
						fPendingRequests[url].push_back(item);
					} catch (...) {
						// Ignore map insertion failure, item stays in list
					}

					if (fRequestListView->CountItems() > 500) {
						NetworkRequestItem* oldItem
							= (NetworkRequestItem*)fRequestListView->ItemAt(0);
						if (oldItem->IsPending()) {
							std::deque<NetworkRequestItem*>& list
								= fPendingRequests[oldItem->Url()];
							if (!list.empty() && list.front() == oldItem) {
								list.pop_front();
								if (list.empty())
									fPendingRequests.erase(oldItem->Url());
							}
						}
						delete fRequestListView->RemoveItem(0);
					}
					fRequestListView->ScrollTo(fRequestListView->CountItems() - 1);
				} else {
					delete item;
				}
			}
			break;
		}
		case UPDATE_NETWORK_REQUEST:
					NetworkRequestItem* oldItem
						= (NetworkRequestItem*)fRequestListView->ItemAt(0);
					if (oldItem->IsPending()) {
						std::deque<NetworkRequestItem*>& list
							= fPendingRequests[oldItem->Url()];
						if (!list.empty() && list.front() == oldItem) {
							list.pop_front();
							if (list.empty())
								fPendingRequests.erase(oldItem->Url());
						}
					}
					delete fRequestListView->RemoveItem(0);
				}
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

				std::map<BString, std::deque<NetworkRequestItem*> >::iterator it
					= fPendingRequests.find(url);
				if (it != fPendingRequests.end() && !it->second.empty()) {
					NetworkRequestItem* item = it->second.front();

					BString displayText;
					displayText.SetToFormat("[%s] %s (Headers: N/A)",
						status.String(), url.String());
					item->SetText(displayText);
					item->SetPending(false);

					fRequestListView->InvalidateItem(
						fRequestListView->IndexOf(item));

					it->second.pop_front();
					if (it->second.empty())
						fPendingRequests.erase(it);
				}
			}
			break;
		}
		case CLEAR_NETWORK_REQUESTS:
		{
			fPendingRequests.clear();
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
	if (fQuitting)
		return true;

	if (!IsHidden())
		Hide();
	return false;
}


void
NetworkWindow::SetTarget(const BMessenger& target)
{
	fTarget = target;
}


void
NetworkWindow::PrepareToQuit()
{
	fQuitting = true;
}
