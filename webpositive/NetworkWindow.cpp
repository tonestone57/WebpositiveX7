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
	// The list view is just a view, the data is owned by fAllRequests
	fRequestListView->SetOwnership(false);

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
		fTarget.SendMessage(NETWORK_WINDOW_CLOSED);

	fPendingRequests.clear();
	// Items in fAllRequests own the memory
	for (std::deque<NetworkRequestItem*>::iterator it = fAllRequests.begin();
			it != fAllRequests.end(); ++it) {
		delete *it;
	}
	fAllRequests.clear();
	fRequestListView->MakeEmpty(); // List view doesn't own items anymore
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
				_AppendRequest(item);
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

					if (!IsHidden()) {
						fRequestListView->InvalidateItem(
							fRequestListView->IndexOf(item));
					}

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
			for (std::deque<NetworkRequestItem*>::iterator it = fAllRequests.begin();
					it != fAllRequests.end(); ++it) {
				delete *it;
			}
			fAllRequests.clear();
			fRequestListView->MakeEmpty();
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
NetworkWindow::Show()
{
	if (IsHidden())
		_UpdateList();
	BWindow::Show();
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


void
NetworkWindow::_AppendRequest(NetworkRequestItem* item)
{
	try {
		fAllRequests.push_back(item);
		try {
			fPendingRequests[item->Url()].push_back(item);
		} catch (...) {
			fAllRequests.pop_back();
			throw;
		}
	} catch (...) {
		delete item;
		return;
	}

	if (fAllRequests.size() > 500) {
		NetworkRequestItem* oldItem = fAllRequests.front();
		if (oldItem->IsPending()) {
			std::deque<NetworkRequestItem*>& list
				= fPendingRequests[oldItem->Url()];
			for (std::deque<NetworkRequestItem*>::iterator it = list.begin();
					it != list.end(); ++it) {
				if (*it == oldItem) {
					list.erase(it);
					break;
				}
			}
			if (list.empty())
				fPendingRequests.erase(oldItem->Url());
		}
		fAllRequests.pop_front();

		if (!IsHidden())
			fRequestListView->RemoveItem(0);
		else {
			// If hidden, the list view might contain a stale pointer to oldItem
			// if it was added when the window was visible.
			// To be safe, we clear the list view when hidden to avoid
			// holding dangling pointers. It will be rebuilt in Show().
			if (!fRequestListView->IsEmpty())
				fRequestListView->MakeEmpty();
		}

		delete oldItem;
	}

	if (!IsHidden()) {
		fRequestListView->AddItem(item);
		BRect frame = fRequestListView->ItemFrame(fRequestListView->CountItems() - 1);
		fRequestListView->ScrollTo(0, frame.top);
	}
}


void
NetworkWindow::_UpdateList()
{
	fRequestListView->MakeEmpty();
	for (std::deque<NetworkRequestItem*>::iterator it = fAllRequests.begin();
			it != fAllRequests.end(); ++it) {
		fRequestListView->AddItem(*it);
	}
	int32 count = fRequestListView->CountItems();
	if (count > 0) {
		BRect frame = fRequestListView->ItemFrame(count - 1);
		fRequestListView->ScrollTo(0, frame.top);
	}
}
