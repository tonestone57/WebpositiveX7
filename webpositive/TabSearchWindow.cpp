/*
 * Copyright 2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "TabSearchWindow.h"

#include "BrowserWindow.h"

#include <Application.h>
#include <Autolock.h>
#include <GroupLayoutBuilder.h>
#include <ListView.h>
#include <ScrollView.h>
#include <TextControl.h>
#include <StringItem.h>

#include "TabManager.h"

enum {
	MSG_SEARCH_TEXT_CHANGED = 'mstc',
	MSG_TAB_SELECTED = 'mtsl'
};


class TabListItem : public BStringItem {
public:
	TabListItem(const char* text, BView* view)
		: BStringItem(text), fTabView(view) {}

	BView* View() const { return fTabView; }

private:
	BView* fTabView;
};


TabSearchWindow::TabSearchWindow(TabManager* manager)
	:
	BWindow(BRect(0, 0, 300, 400), "Search Tabs", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fTabManager(manager),
	fTarget(manager->Target()),
	fForceQuit(false)
{
	CenterOnScreen();

	fSearchControl = new BTextControl("search", "Search:", "",
		new BMessage(MSG_SEARCH_TEXT_CHANGED));
	fSearchControl->SetModificationMessage(new BMessage(MSG_SEARCH_TEXT_CHANGED));

	fTabList = new BListView("tabList");
	fTabList->SetSelectionMessage(new BMessage(MSG_TAB_SELECTED));

	SetLayout(new BGroupLayout(B_VERTICAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(fSearchControl)
		.Add(new BScrollView("scroll", fTabList, 0, false, true))
		.SetInsets(10, 10, 10, 10)
	);

	_UpdateList();
	fSearchControl->MakeFocus(true);
}


TabSearchWindow::~TabSearchWindow()
{
	for (int32 i = fTabList->CountItems() - 1; i >= 0; i--)
		delete fTabList->RemoveItem(i);
}


void
TabSearchWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SEARCH_TEXT_CHANGED:
			_FilterList();
			break;

		case MSG_TAB_SELECTED:
		{
			int32 selection = fTabList->CurrentSelection();
			if (selection >= 0) {
				TabListItem* item = dynamic_cast<TabListItem*>(fTabList->ItemAt(selection));
				if (item) {
					BView* view = item->View();
					if (fTarget.IsValid()) {
						BMessage selectMsg(SELECT_TAB_BY_VIEW);
						selectMsg.AddPointer("view", view);
						fTarget.SendMessage(&selectMsg);
						PostMessage(B_QUIT_REQUESTED);
					}
				}
			}
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
TabSearchWindow::QuitRequested()
{
	if (fForceQuit)
		return true;

	if (fTarget.IsValid()) {
		BMessage msg(TAB_SEARCH_WINDOW_QUIT);
		fTarget.SendMessage(&msg);
	}
	Hide();
	return false;
}


void
TabSearchWindow::PrepareToQuit()
{
	fForceQuit = true;
}


void
TabSearchWindow::SetTabManager(TabManager* manager)
{
	fTabManager = manager;
}


void
TabSearchWindow::_UpdateList()
{
	for (int32 i = fTabList->CountItems() - 1; i >= 0; i--)
		delete fTabList->RemoveItem(i);

	if (!fTabManager) return;

	BMessenger target = fTarget;
	BLooper* looper;
	target.Target(&looper);
	// If we can't lock, we assume the window is busy or dead, so we skip updating
	if (looper && looper->LockWithTimeout(100000) == B_OK) {
		for (int32 i = 0; i < fTabManager->CountTabs(); i++) {
			BString label = fTabManager->TabLabel(i);
			BView* view = fTabManager->ViewForTab(i);
			fTabList->AddItem(new TabListItem(label.String(), view));
		}
		looper->Unlock();
	}
}


void
TabSearchWindow::_FilterList()
{
	if (!fTabManager) return;

	BMessenger target = fTarget;
	BLooper* looper;
	target.Target(&looper);
	if (looper && looper->LockWithTimeout(100000) == B_OK) {
		BString filter = fSearchControl->Text();
		for (int32 i = fTabList->CountItems() - 1; i >= 0; i--)
			delete fTabList->RemoveItem(i);

		for (int32 i = 0; i < fTabManager->CountTabs(); i++) {
			BString label = fTabManager->TabLabel(i);
			BView* view = fTabManager->ViewForTab(i);
			if (filter.Length() == 0 || label.IFindFirst(filter) >= 0) {
				fTabList->AddItem(new TabListItem(label.String(), view));
			}
		}
		looper->Unlock();
	}
}
