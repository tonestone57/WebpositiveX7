/*
 * Copyright 2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "PermissionsWindow.h"

#include <Application.h>
#include <Button.h>
#include <CheckBox.h>
#include <GroupLayoutBuilder.h>
#include <ListView.h>
#include <ScrollView.h>
#include <TextControl.h>
#include <StringItem.h>
#include <SettingsMessage.h>
#include <FindDirectory.h>
#include <Path.h>

#include "BrowserApp.h"
#include "SitePermissionsManager.h"
#include "BrowsingHistory.h"
#include <NetworkCookie.h>

enum {
	MSG_DOMAIN_SELECTED = 'mdsl',
	MSG_ADD_DOMAIN = 'madd',
	MSG_REMOVE_DOMAIN = 'mrem',
	MSG_SAVE_PERMISSIONS = 'msvp',
	MSG_CLEAR_SITE_DATA = 'clsd'
};

class PermissionItem : public BStringItem {
public:
	PermissionItem(const char* text, bool js, bool cookies, bool popups)
		: BStringItem(text), fJS(js), fCookies(cookies), fPopups(popups) {}

	void SetJS(bool enable) { fJS = enable; }
	bool JS() const { return fJS; }

	void SetCookies(bool enable) { fCookies = enable; }
	bool Cookies() const { return fCookies; }

	void SetPopups(bool enable) { fPopups = enable; }
	bool Popups() const { return fPopups; }

private:
	bool fJS;
	bool fCookies;
	bool fPopups;
};

PermissionsWindow::PermissionsWindow(BRect frame, BPrivate::Network::BNetworkCookieJar& jar)
	:
	BWindow(frame, "Site Permissions", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fCookieJar(jar)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	fDomainList = new BListView("domainList");
	fDomainList->SetSelectionMessage(new BMessage(MSG_DOMAIN_SELECTED));

	fAddDomainControl = new BTextControl("domain", "Domain:", "", NULL);
	fAddButton = new BButton("Add", new BMessage(MSG_ADD_DOMAIN));
	fRemoveButton = new BButton("Remove", new BMessage(MSG_REMOVE_DOMAIN));
	fRemoveButton->SetEnabled(false);

	fClearDataButton = new BButton("Clear Site Data", new BMessage(MSG_CLEAR_SITE_DATA));
	fClearDataButton->SetEnabled(false);

	fJSEnabled = new BCheckBox("Allow JavaScript", new BMessage(MSG_SAVE_PERMISSIONS));
	fCookiesEnabled = new BCheckBox("Allow Cookies", new BMessage(MSG_SAVE_PERMISSIONS));
	fPopupsEnabled = new BCheckBox("Allow Popups", new BMessage(MSG_SAVE_PERMISSIONS));

	// Default state (disabled until domain selected)
	fJSEnabled->SetEnabled(false);
	fCookiesEnabled->SetEnabled(false);
	fPopupsEnabled->SetEnabled(false);

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(new BScrollView("scroll", fDomainList, 0, false, true))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10)
			.Add(fAddDomainControl)
			.Add(fAddButton)
			.Add(fRemoveButton)
		)
		.Add(fJSEnabled)
		.Add(fCookiesEnabled)
		.Add(fPopupsEnabled)
		.Add(fClearDataButton)
		.SetInsets(10)
	);

	_LoadPermissions();
	CenterOnScreen();
}


PermissionsWindow::~PermissionsWindow()
{
}


void
PermissionsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_DOMAIN_SELECTED:
			_UpdateFields();
			break;

		case MSG_ADD_DOMAIN:
		{
			BString domain = fAddDomainControl->Text();
			if (domain.Length() > 0) {
				fDomainList->AddItem(new BStringItem(domain));
				fAddDomainControl->SetText("");
				_SavePermissions();
			}
			break;
		}

		case MSG_REMOVE_DOMAIN:
		{
			int32 selection = fDomainList->CurrentSelection();
			if (selection >= 0) {
				delete fDomainList->RemoveItem(selection);
				_SavePermissions();
				_UpdateFields();
			}
			break;
		}

		case MSG_SAVE_PERMISSIONS:
		{
			int32 selection = fDomainList->CurrentSelection();
			if (selection >= 0) {
				PermissionItem* item = dynamic_cast<PermissionItem*>(fDomainList->ItemAt(selection));
				if (item) {
					item->SetJS(fJSEnabled->Value() == B_CONTROL_ON);
					item->SetCookies(fCookiesEnabled->Value() == B_CONTROL_ON);
					item->SetPopups(fPopupsEnabled->Value() == B_CONTROL_ON);
				}
			}
			_SavePermissions();
			break;
		}

		case MSG_CLEAR_SITE_DATA:
		{
			int32 selection = fDomainList->CurrentSelection();
			if (selection >= 0) {
				PermissionItem* item = dynamic_cast<PermissionItem*>(fDomainList->ItemAt(selection));
				if (item) {
					_ClearSiteData(item->Text());
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
PermissionsWindow::QuitRequested()
{
	return true;
}


// Helper to manage BMessage per domain
static void
SetDomainSettings(BMessage& msg, bool js, bool cookies, bool popups)
{
	msg.RemoveName("js");
	msg.RemoveName("cookies");
	msg.RemoveName("popups");
	msg.AddBool("js", js);
	msg.AddBool("cookies", cookies);
	msg.AddBool("popups", popups);
}

void
PermissionsWindow::_LoadPermissions()
{
	fDomainList->MakeEmpty();
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(kApplicationName);
		path.Append("SitePermissions");
		SettingsMessage settings(B_USER_SETTINGS_DIRECTORY, path.Path());

		int32 i = 0;
		BMessage domainMsg;
		while (settings.FindMessage("domain", i++, &domainMsg) == B_OK) {
			BString domain;
			if (domainMsg.FindString("name", &domain) == B_OK) {
				bool js = true;
				bool cookies = true;
				bool popups = false;
				domainMsg.FindBool("js", &js);
				domainMsg.FindBool("cookies", &cookies);
				domainMsg.FindBool("popups", &popups);

				PermissionItem* item = new PermissionItem(domain, js, cookies, popups);
				fDomainList->AddItem(item);
			}
		}
	}
}


void
PermissionsWindow::_SavePermissions()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(kApplicationName);
		path.Append("SitePermissions");
		SettingsMessage settings(B_USER_SETTINGS_DIRECTORY, path.Path());
		settings.MakeEmpty();

		for (int32 i = 0; i < fDomainList->CountItems(); i++) {
			PermissionItem* item = dynamic_cast<PermissionItem*>(fDomainList->ItemAt(i));
			if (!item) continue;

			BString domain = item->Text();
			BMessage domainMsg;
			domainMsg.AddString("name", domain);

			SetDomainSettings(domainMsg,
				item->JS(),
				item->Cookies(),
				item->Popups());

			settings.AddMessage("domain", &domainMsg);
		}
		settings.Save();
		SitePermissionsManager::Instance()->Reload();
	}
}


void
PermissionsWindow::_UpdateFields()
{
	int32 selection = fDomainList->CurrentSelection();
	bool hasSelection = selection >= 0;

	fRemoveButton->SetEnabled(hasSelection);
	fJSEnabled->SetEnabled(hasSelection);
	fCookiesEnabled->SetEnabled(hasSelection);
	fPopupsEnabled->SetEnabled(hasSelection);
	fClearDataButton->SetEnabled(hasSelection);

	if (hasSelection) {
		PermissionItem* item = dynamic_cast<PermissionItem*>(fDomainList->ItemAt(selection));
		if (item) {
			fJSEnabled->SetValue(item->JS() ? B_CONTROL_ON : B_CONTROL_OFF);
			fCookiesEnabled->SetValue(item->Cookies() ? B_CONTROL_ON : B_CONTROL_OFF);
			fPopupsEnabled->SetValue(item->Popups() ? B_CONTROL_ON : B_CONTROL_OFF);
		}
	}
}

void
PermissionsWindow::_ClearSiteData(const char* domain)
{
	// 1. Clear Cookies
	BPrivate::Network::BNetworkCookieJar::Iterator it = fCookieJar.GetIterator();
	const BPrivate::Network::BNetworkCookie* cookie;
	// We need to collect cookies to delete first, to avoid invalidating iterator
	// or modifying collection while iterating (though iterator is const)
	// BNetworkCookieJar doesn't support "remove by domain" directly usually.
	// Actually BNetworkCookieJar doesn't seem to expose RemoveCookie clearly in this scope?
	// The CookieWindow logic sets expiration to 0 to delete.

	// Re-iterate to find cookies for domain
	// Note: Cookie domains can start with '.'
	BString targetDomain(domain);

	// Collect cookies to "delete" (expire)
	std::vector<BPrivate::Network::BNetworkCookie> cookiesToDelete;

	while ((cookie = it.Next()) != NULL) {
		BString cookieDomain = cookie->Domain();
		// Match exact or subdomain (preceded by dot)
		// e.g. target "example.com" matches "example.com" and ".example.com" and "foo.example.com"
		// but NOT "myexample.com"
		if (cookieDomain == targetDomain ||
			(cookieDomain.EndsWith(targetDomain) &&
			 cookieDomain.Length() > targetDomain.Length() &&
			 cookieDomain[cookieDomain.Length() - targetDomain.Length() - 1] == '.')) {
			cookiesToDelete.push_back(*cookie);
		}
	}

	for (size_t i = 0; i < cookiesToDelete.size(); i++) {
		BPrivate::Network::BNetworkCookie c = cookiesToDelete[i];
		c.SetExpirationDate(0);
		fCookieJar.AddCookie(c);
	}

	// 2. Clear History
	BrowsingHistory* history = BrowsingHistory::DefaultInstance();
	if (history->Lock()) {
		// BrowsingHistory doesn't support iteration easily from outside without locking issues if we modify?
		// But RemoveUrl is O(log N). We need to find URLs matching the domain.
		// History stores URLs.

		int32 count = history->CountItems();
		std::vector<BString> urlsToRemove;
		for (int32 i = 0; i < count; i++) {
			BrowsingHistoryItem item = history->HistoryItemAt(i);
			BUrl url(item.URL());
			if (url.Host() == targetDomain ||
				(url.Host().EndsWith(targetDomain) &&
				 url.Host().Length() > targetDomain.Length() &&
				 url.Host()[url.Host().Length() - targetDomain.Length() - 1] == '.')) {
				urlsToRemove.push_back(item.URL());
			}
		}

		history->Unlock();
		// Re-lock for modification if needed, or modify while locked if safe.
		// RemoveUrl takes a lock internally? Let's check BrowsingHistory.
		// Assuming we should just call RemoveUrl outside the loop if it locks.

		for (size_t i = 0; i < urlsToRemove.size(); i++) {
			history->RemoveUrl(urlsToRemove[i]);
		}
	}
}
