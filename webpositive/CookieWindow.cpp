/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues
 */


#include "CookieWindow.h"

#include <Button.h>
#include <Catalog.h>
#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <FilePanel.h>
#include <GroupLayoutBuilder.h>
#include <NetworkCookieJar.h>
#include <OutlineListView.h>
#include <ScrollView.h>
#include <StringView.h>

#include "Sync.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Cookie Manager"

enum {
	COOKIE_IMPORT = 'cimp',
	COOKIE_EXPORT = 'cexp',
	COOKIE_DELETE = 'cdel',
	COOKIE_REFRESH = 'rfsh',

	DOMAIN_SELECTED = 'dmsl'
};


class CookieDateColumn: public BDateColumn
{
public:
	CookieDateColumn(const char* title, float width)
		:
		BDateColumn(title, width, width / 2, width * 2)
	{
	}

	void DrawField(BField* field, BRect rect, BView* parent) {
		BDateField* dateField = (BDateField*)field;
		if (dateField->UnixTime() == -1) {
			DrawString(B_TRANSLATE("Session cookie"), parent, rect);
		} else {
			BDateColumn::DrawField(field, rect, parent);
		}
	}
};


class CookieRow: public BRow
{
public:
	CookieRow(BColumnListView* list,
		const BPrivate::Network::BNetworkCookie& cookie)
		:
		BRow(),
		fCookie(cookie)
	{
		list->AddRow(this);
		SetField(new BStringField(cookie.Name().String()), 0);
		SetField(new BStringField(cookie.Path().String()), 1);
		time_t expiration = cookie.ExpirationDate();
		SetField(new BDateField(&expiration), 2);
		SetField(new BStringField(cookie.Value().String()), 3);

		BString flags;
		if (cookie.Secure())
			flags = "https ";
		if (cookie.HttpOnly())
			flags = "http ";

		if (cookie.IsHostOnly())
			flags += "hostOnly";
		SetField(new BStringField(flags.String()), 4);
	}

	BPrivate::Network::BNetworkCookie& Cookie() {
		return fCookie;
	}

private:
	BPrivate::Network::BNetworkCookie	fCookie;
};


class DomainItem: public BStringItem
{
public:
	DomainItem(BString text, bool empty)
		:
		BStringItem(text),
		fEmpty(empty)
	{
	}

public:
	bool	fEmpty;
};


CookieWindow::CookieWindow(BRect frame,
	BPrivate::Network::BNetworkCookieJar& jar)
	:
	BWindow(frame, B_TRANSLATE("Cookie manager"), B_TITLED_WINDOW,
		B_NORMAL_WINDOW_FEEL,
		B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE),
	fCookieJar(jar)
{
	BGroupLayout* root = new BGroupLayout(B_HORIZONTAL, 0.0);
	SetLayout(root);

	fDomains = new BOutlineListView("domain list");
	root->AddView(new BScrollView("scroll", fDomains, 0, false, true), 1);

	fHeaderView = new BStringView("label",
		B_TRANSLATE("The cookie jar is empty!"));
	fCookies = new BColumnListView("cookie list", B_WILL_DRAW, B_FANCY_BORDER,
		false);

	int em = fCookies->StringWidth("M");
	int flagsLength = fCookies->StringWidth("Mhttps hostOnly" B_UTF8_ELLIPSIS);

	fCookies->AddColumn(new BStringColumn(B_TRANSLATE("Name"),
		20 * em, 10 * em, 50 * em, 0), 0);
	fCookies->AddColumn(new BStringColumn(B_TRANSLATE("Path"),
		10 * em, 10 * em, 50 * em, 0), 1);
	fCookies->AddColumn(new CookieDateColumn(B_TRANSLATE("Expiration"),
		fCookies->StringWidth("88/88/8888 88:88:88 AM")), 2);
	fCookies->AddColumn(new BStringColumn(B_TRANSLATE("Value"),
		20 * em, 10 * em, 50 * em, 0), 3);
	fCookies->AddColumn(new BStringColumn(B_TRANSLATE("Flags"),
		flagsLength, flagsLength, flagsLength, 0), 4);

	root->AddItem(BGroupLayoutBuilder(B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(5, 5, 5, 5)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fHeaderView)
			.AddGlue()
		.End()
		.Add(fCookies)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.SetInsets(5, 5, 5, 5)
			.Add(new BButton("import", B_TRANSLATE("Import" B_UTF8_ELLIPSIS),
				new BMessage(COOKIE_IMPORT)))
			.Add(new BButton("export", B_TRANSLATE("Export" B_UTF8_ELLIPSIS),
				new BMessage(COOKIE_EXPORT)))
			.AddGlue()
			.Add(new BButton("delete", B_TRANSLATE("Delete"),
				new BMessage(COOKIE_DELETE))), 3);

	fDomains->SetSelectionMessage(new BMessage(DOMAIN_SELECTED));
}


void
CookieWindow::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case DOMAIN_SELECTED:
		{
			int32 index = message->FindInt32("index");
			BStringItem* item = (BStringItem*)fDomains->ItemAt(index);
			if (item != NULL) {
				BString domain = item->Text();
				_ShowCookiesForDomain(domain);
			}
			return;
		}

		case COOKIE_REFRESH:
			_BuildDomainList();
			return;

		case COOKIE_DELETE:
			_DeleteCookies();
			return;

		case COOKIE_IMPORT:
		{
			BFilePanel* panel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this),
				NULL, B_FILE_NODE, false, new BMessage(COOKIE_IMPORT));
			panel->Show();
			break;
		}
		case COOKIE_EXPORT:
		{
			BFilePanel* panel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this),
				NULL, 0, false, new BMessage(COOKIE_EXPORT));
			panel->Show();
			break;
		}
		case B_SAVE_REQUESTED:
		{
			if (message->what == B_SAVE_REQUESTED) {
				// Check which export
				// Actually BFilePanel sends B_SAVE_REQUESTED to target.
				// But we attached a message.
				// Wait, BFilePanel(..., msg) -> msg is used for "selection" in OPEN_PANEL,
				// but for SAVE_PANEL it sends B_SAVE_REQUESTED.
				// We need to differentiate if we had multiple save panels.
				// But here we can just assume it is cookie export if we are in CookieWindow.
				// Or we can check the message? BFilePanel doesn't easily attach "userdata" to B_SAVE_REQUESTED.
				// However, we only have one export in CookieWindow.
				entry_ref ref;
				BString name;
				if (message->FindRef("directory", &ref) == B_OK
					&& message->FindString("name", &name) == B_OK) {
					BPath path(&ref);
					path.Append(name);
					Sync::ExportCookies(path, fCookieJar);
				}
			}
			break;
		}
		case B_REFS_RECEIVED:
		{
			// Import
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK) {
				BPath path(&ref);
				Sync::ImportCookies(path, fCookieJar);
				_BuildDomainList();
			}
			break;
		}
	}
	BWindow::MessageReceived(message);
}


void
CookieWindow::Show()
{
	BWindow::Show();
	if (IsHidden())
		return;

	PostMessage(COOKIE_REFRESH);
}


void
CookieWindow::Hide()
{
	BWindow::Hide();
	_EmptyDomainList();
}


bool
CookieWindow::QuitRequested()
{
	if (!IsHidden())
		Hide();
	return false;
}


void
CookieWindow::_EmptyDomainList()
{
	for (int i = fDomains->FullListCountItems() - 1; i >= 1; i--) {
		delete fDomains->FullListItemAt(i);
	}
	fDomains->MakeEmpty();
}


void
CookieWindow::_BuildDomainList()
{
	// NOTE: This function is called every time the window is shown. Attempts to
	// optimize this by caching the list and only rebuilding it when the cookie
	// jar has changed have been unsuccessful. The BNetworkCookieJar API does not
	// provide an efficient way to track changes. A simple cookie count is not
	// enough, as it won't detect changes to existing cookies (e.g. value or
	// expiration date). A more robust solution would require changes to the
	// BNetworkCookieJar API itself.

	_EmptyDomainList();

	// BOutlineListView does not handle parent = NULL in many methods, so let's
	// make sure everything always has a parent.
	DomainItem* rootItem = new DomainItem("", true);
	fDomains->AddItem(rootItem);

	// Populate the domain list
	fCookieMap.clear();
	std::map<BString, BStringItem*> domainItemMap;

	BPrivate::Network::BNetworkCookieJar::Iterator it = fCookieJar.GetIterator();

	const BPrivate::Network::BNetworkCookie* cookie;
	while ((cookie = it.Next()) != NULL) {
		BString domain = cookie->Domain();
		if (fCookieMap.find(domain) == fCookieMap.end())
			_AddDomain(domain, false, domainItemMap);

		fCookieMap[domain].push_back(*cookie);
	}

	int i = 1;
	while (i < fDomains->FullListCountItems())
	{
		DomainItem* item = (DomainItem*)fDomains->FullListItemAt(i);
		// Detach items from the fake root
		item->SetOutlineLevel(item->OutlineLevel() - 1);
		i++;
	}
	fDomains->RemoveItem(rootItem);
	delete rootItem;

	i = 0;
	int firstNotEmpty = i;
	// Collapse empty items to keep the list short
	while (i < fDomains->FullListCountItems())
	{
		DomainItem* item = (DomainItem*)fDomains->FullListItemAt(i);
		if (item->fEmpty == true) {
			if (fDomains->CountItemsUnder(item, true) == 1) {
				// The item has no cookies, and only a single child. We can
				// remove it and move its child one level up in the tree.

				int count = fDomains->CountItemsUnder(item, false);
				int index = fDomains->FullListIndexOf(item) + 1;
				for (int j = 0; j < count; j++) {
					BListItem* child = fDomains->FullListItemAt(index + j);
					child->SetOutlineLevel(child->OutlineLevel() - 1);
				}

				fDomains->RemoveItem(item);
				delete item;

				// The moved child is at the same index the removed item was.
				// We continue the loop without incrementing i to process it.
				continue;
			} else {
				// The item has no cookies, but has multiple children. Mark it
				// as disabled so it is not selectable.
				item->SetEnabled(false);
				if (i == firstNotEmpty)
					firstNotEmpty++;
			}
		}

		i++;
	}

	fDomains->Select(firstNotEmpty);
}


BStringItem*
CookieWindow::_AddDomain(BString domain, bool fake,
	std::map<BString, BStringItem*>& domainMap)
{
	if (domainMap.count(domain) != 0) {
		DomainItem* item = (DomainItem*)domainMap[domain];
		if (!fake)
			item->fEmpty = false;
		return item;
	}

	BStringItem* parent = NULL;
	int firstDot = domain.FindFirst('.');
	if (firstDot >= 0) {
		BString parentDomain(domain);
		parentDomain.Remove(0, firstDot + 1);
		parent = _AddDomain(parentDomain, true, domainMap);
	} else {
		parent = (BStringItem*)fDomains->FullListItemAt(0);
	}

	int i = 0;
#if 0
	puts("==============================");
	for (i = 0; i < fDomains->FullListCountItems(); i++) {
		BStringItem* t = (BStringItem*)fDomains->FullListItemAt(i);
		for (unsigned j = 0; j < t->OutlineLevel(); j++)
			printf("  ");
		printf("%s\n", t->Text());
	}
#endif

	// Insert the new item, keeping the list alphabetically sorted
	BStringItem* domainItem = new DomainItem(domain, fake);
	domainMap[domain] = domainItem;
	domainItem->SetOutlineLevel(parent->OutlineLevel() + 1);
	BStringItem* sibling = NULL;
	int siblingCount = fDomains->CountItemsUnder(parent, true);
	for (i = 0; i < siblingCount; i++) {
		sibling = (BStringItem*)fDomains->ItemUnderAt(parent, true, i);
		if (strcmp(sibling->Text(), domainItem->Text()) > 0) {
			fDomains->AddItem(domainItem, fDomains->FullListIndexOf(sibling));
			return domainItem;
		}
	}

	if (sibling) {
		// There were siblings, but all smaller than what we try to insert.
		// Insert after the last one (and its subitems)
		fDomains->AddItem(domainItem, fDomains->FullListIndexOf(sibling)
			+ fDomains->CountItemsUnder(sibling, false) + 1);
	} else {
		// There were no siblings, insert right after the parent
		fDomains->AddItem(domainItem, fDomains->FullListIndexOf(parent) + 1);
	}

	return domainItem;
}


void
CookieWindow::_ShowCookiesForDomain(BString domain)
{
	BString label;
	label.SetToFormat(B_TRANSLATE("Cookies for %s"), domain.String());
	fHeaderView->SetText(label);

	// Empty the cookie list
	fCookies->Clear();

	if (fCookieMap.find(domain) == fCookieMap.end())
		return;

	const std::vector<BPrivate::Network::BNetworkCookie>& cookies
		= fCookieMap[domain];
	std::vector<BPrivate::Network::BNetworkCookie>::const_iterator it;
	for (it = cookies.begin(); it != cookies.end(); ++it)
		new CookieRow(fCookies, *it);
}


void
CookieWindow::_DeleteCookies()
{
	CookieRow* row;
	CookieRow* prevRow;

	for (prevRow = NULL; ; prevRow = row) {
		row = (CookieRow*)fCookies->CurrentSelection(prevRow);

		if (prevRow != NULL) {
			fCookies->RemoveRow(prevRow);
			delete prevRow;
		}

		if (row == NULL)
			break;

		// delete this cookie
		BPrivate::Network::BNetworkCookie& cookie = row->Cookie();
		cookie.SetExpirationDate(0);
		fCookieJar.AddCookie(cookie);
	}

	// A domain was selected in the domain list
	if (prevRow == NULL) {
		int32 count = fCookies->CountRows();
		for (int32 i = 0; i < count; i++) {
			row = (CookieRow*)fCookies->RowAt(i);
			BPrivate::Network::BNetworkCookie& cookie = row->Cookie();
			cookie.SetExpirationDate(0);
			fCookieJar.AddCookie(cookie);
		}
		fCookies->Clear();
	}

	PostMessage(COOKIE_REFRESH);
}
