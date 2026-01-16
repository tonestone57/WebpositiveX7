/*
 * Copyright 2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "TabSearchWindow.h"

#include <Application.h>
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
	fTabManager(manager)
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
		.SetInsets(10)
	);

	_UpdateList();
	fSearchControl->MakeFocus(true);
}


TabSearchWindow::~TabSearchWindow()
{
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
					if (fTabManager->HasView(view)) {
						fTabManager->SelectTab(view);
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
	if (fTabManager) {
		BMessage msg('tswq'); // TAB_SEARCH_WINDOW_QUIT
		fTabManager->Target().SendMessage(&msg);
	}
	return true;
}


void
TabSearchWindow::_UpdateList()
{
	fTabList->MakeEmpty();
	if (!fTabManager) return;

	for (int32 i = 0; i < fTabManager->CountTabs(); i++) {
		BString label = fTabManager->TabLabel(i);
		BView* view = fTabManager->ViewForTab(i);
		fTabList->AddItem(new TabListItem(label.String(), view));
	}
}


void
TabSearchWindow::_FilterList()
{
	BString filter = fSearchControl->Text();
	fTabList->MakeEmpty();

	for (int32 i = 0; i < fTabManager->CountTabs(); i++) {
		BString label = fTabManager->TabLabel(i);
		BView* view = fTabManager->ViewForTab(i);
		if (filter.Length() == 0 || label.IFindFirst(filter) >= 0) {
			fTabList->AddItem(new TabListItem(label.String(), view));
		}
	}
}
