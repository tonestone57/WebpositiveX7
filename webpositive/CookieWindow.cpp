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

	DOMAIN_SELECTED = 'dmsl',
	COOKIE_SELECTED = 'cksl'
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


struct DomainNode {
	BString domain;
	bool fake;
	std::map<BString, DomainNode*> children;

	DomainNode(const BString& d, bool f)
		:
		domain(d),
		fake(f)
	{
	}

	~DomainNode()
	{
		std::map<BString, DomainNode*>::iterator it;
		for (it = children.begin(); it != children.end(); it++) {
			delete it->second;
		}
	}
};


CookieWindow::CookieWindow(BRect frame,
	BPrivate::Network::BNetworkCookieJar& jar)
	:
	BWindow(frame, B_TRANSLATE("Cookie manager"), B_TITLED_WINDOW,
		B_NORMAL_WINDOW_FEEL,
		B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE),
	fQuitting(false),
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

	fDeleteButton = new BButton("delete", B_TRANSLATE("Delete"),
		new BMessage(COOKIE_DELETE));
	fDeleteButton->SetEnabled(false);

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
			.Add(fDeleteButton), 3);

	fDomains->SetSelectionMessage(new BMessage(DOMAIN_SELECTED));
	fCookies->SetSelectionMessage(new BMessage(COOKIE_SELECTED));
}


CookieWindow::~CookieWindow()
{
	_EmptyDomainList();
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
			fDeleteButton->SetEnabled(index >= 0);
			return;
		}

		case COOKIE_SELECTED:
		{
			fDeleteButton->SetEnabled(fCookies->CurrentSelection() != NULL ||
				fDomains->CurrentSelection() >= 0);
			return;
		}

		case COOKIE_REFRESH:
			_BuildDomainList();
			fDeleteButton->SetEnabled(false);
			return;

		case COOKIE_DELETE:
			_DeleteCookies();
			return;

		case COOKIE_IMPORT:
		{
			if (fImportPanel == NULL) {
				fImportPanel.reset(new BFilePanel(B_OPEN_PANEL, new BMessenger(this),
					NULL, B_FILE_NODE, false, new BMessage(COOKIE_IMPORT)));
			}
			fImportPanel->Show();
			break;
		}
		case COOKIE_EXPORT:
		{
			if (fExportPanel == NULL) {
				fExportPanel.reset(new BFilePanel(B_SAVE_PANEL, new BMessenger(this),
					NULL, 0, false, new BMessage(COOKIE_EXPORT)));
			}
			fExportPanel->Show();
			break;
		}
		case B_SAVE_REQUESTED:
		{
			if (message->what == B_SAVE_REQUESTED) {
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
	if (fQuitting)
		return true;

	if (!IsHidden())
		Hide();
	return false;
}


void
CookieWindow::PrepareToQuit()
{
	fQuitting = true;
}


void
CookieWindow::_EmptyDomainList()
{
	for (int32 i = fDomains->FullListCountItems() - 1; i >= 0; i--) {
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

	// Populate the domain list
	fCookieMap.clear();

	// 1. Build a tree of domains in memory
	std::unique_ptr<DomainNode> rootNode(new DomainNode("", true));

	BPrivate::Network::BNetworkCookieJar::Iterator it = fCookieJar.GetIterator();
	const BPrivate::Network::BNetworkCookie* cookie;

	while ((cookie = it.Next()) != NULL) {
		BString domain = cookie->Domain();
		fCookieMap[domain].push_back(*cookie);
	}

	std::map<BString, std::vector<BPrivate::Network::BNetworkCookie> >::iterator mapIt;
	for (mapIt = fCookieMap.begin(); mapIt != fCookieMap.end(); ++mapIt) {
		BString domain = mapIt->first;

		// Decompose domain into parts to build the tree.
		// We want to process from root down to leaf.
		// e.g. "mail.google.com" -> ["com", "google.com", "mail.google.com"]

		std::vector<BString> path;
		BString temp = domain;
		while (true) {
			path.push_back(temp);
			int firstDot = temp.FindFirst('.');
			if (firstDot < 0) break;
			temp.Remove(0, firstDot + 1);
		}

		DomainNode* current = rootNode.get();
		// Iterate backwards (from "com" to "mail.google.com")
		for (int i = path.size() - 1; i >= 0; i--) {
			BString& part = path[i];
			if (current->children.find(part) == current->children.end()) {
				bool isLeaf = (i == 0);
				// If it is the full domain of the cookie, mark it as real (not fake).
				// Otherwise, it's an intermediate node (fake) unless we find a cookie for it later.
				DomainNode* newNode = new DomainNode(part, !isLeaf);
				current->children[part] = newNode;
				current = newNode;
			} else {
				current = current->children[part];
				if (i == 0) {
					// We found a cookie for this node, so it's a real domain now.
					current->fake = false;
				}
			}
		}
	}

	// 2. Traverse the tree and populate the list
	// We use a recursive helper class to flatten the tree into the BOutlineListView
	struct TreeFlattener {
		static void Flatten(DomainNode* node, BOutlineListView* list, int level) {
			std::map<BString, DomainNode*>::iterator it;
			for (it = node->children.begin(); it != node->children.end(); it++) {
				DomainNode* child = it->second;
				DomainItem* item = new DomainItem(child->domain, child->fake);
				if (item == NULL)
					continue;
				item->SetOutlineLevel(level);
				if (!list->AddItem(item)) {
					delete item;
					continue;
				}
				Flatten(child, list, level + 1);
			}
		}
	};

	TreeFlattener::Flatten(rootNode.get(), fDomains, 0);

	int32 i = 0;
	int32 firstNotEmpty = i;
	// Collapse empty items to keep the list short
	while (i < fDomains->FullListCountItems())
	{
		DomainItem* item = (DomainItem*)fDomains->FullListItemAt(i);
		if (item->fEmpty == true) {
			if (fDomains->CountItemsUnder(item, true) == 1) {
				// The item has no cookies, and only a single child. We can
				// remove it and move its child one level up in the tree.

				int32 count = fDomains->CountItemsUnder(item, false);
				int32 index = fDomains->FullListIndexOf(item) + 1;
				for (int32 j = 0; j < count; j++) {
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

	if (firstNotEmpty < fDomains->FullListCountItems())
		fDomains->Select(firstNotEmpty);
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
	std::vector<CookieRow*> rowsToDelete;

	for (row = (CookieRow*)fCookies->CurrentSelection(NULL); row != NULL;
			row = (CookieRow*)fCookies->CurrentSelection(row)) {
		rowsToDelete.push_back(row);
	}

	if (rowsToDelete.empty()) {
		// A domain was selected in the domain list, but no specific cookies
		// selected -> delete all cookies for this domain.
		int32 count = fCookies->CountRows();
		for (int32 i = 0; i < count; i++) {
			row = (CookieRow*)fCookies->RowAt(i);
			BPrivate::Network::BNetworkCookie& cookie = row->Cookie();
			cookie.SetExpirationDate(0);
			fCookieJar.AddCookie(cookie);
		}
		fCookies->Clear();
	} else {
		// Delete selected cookies
		for (size_t i = 0; i < rowsToDelete.size(); i++) {
			row = rowsToDelete[i];
			BPrivate::Network::BNetworkCookie& cookie = row->Cookie();
			cookie.SetExpirationDate(0);
			fCookieJar.AddCookie(cookie);

			fCookies->RemoveRow(row);
			delete row;
		}
	}

	PostMessage(COOKIE_REFRESH);
}
