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
	PermissionItem(const char* text, bool js, bool cookies, bool popups, float zoom, bool forceDesktop, const char* ua)
		: BStringItem(text), fJS(js), fCookies(cookies), fPopups(popups), fZoom(zoom), fForceDesktop(forceDesktop), fCustomUserAgent(ua) {}

	void SetJS(bool enable) { fJS = enable; }
	bool JS() const { return fJS; }

	void SetCookies(bool enable) { fCookies = enable; }
	bool Cookies() const { return fCookies; }

	void SetPopups(bool enable) { fPopups = enable; }
	bool Popups() const { return fPopups; }

	void SetZoom(float zoom) { fZoom = zoom; }
	float Zoom() const { return fZoom; }

	void SetForceDesktop(bool enable) { fForceDesktop = enable; }
	bool ForceDesktop() const { return fForceDesktop; }

	void SetCustomUserAgent(const char* ua) { fCustomUserAgent = ua; }
	const char* CustomUserAgent() const { return fCustomUserAgent.String(); }

private:
	bool fJS;
	bool fCookies;
	bool fPopups;
	float fZoom;
	bool fForceDesktop;
	BString fCustomUserAgent;
};

PermissionsWindow::PermissionsWindow(BRect frame, BPrivate::Network::BNetworkCookieJar& jar)
	:
	BWindow(frame, "Site Permissions", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fCookieJar(jar),
	fQuitting(false)
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
	fForceDesktopCheckBox = new BCheckBox("Force Desktop Mode", new BMessage(MSG_SAVE_PERMISSIONS));

	fZoomControl = new BTextControl("zoom", "Zoom:", "1.0", new BMessage(MSG_SAVE_PERMISSIONS));
	fUserAgentControl = new BTextControl("ua", "User Agent:", "", new BMessage(MSG_SAVE_PERMISSIONS));

	// Default state (disabled until domain selected)
	fJSEnabled->SetEnabled(false);
	fCookiesEnabled->SetEnabled(false);
	fPopupsEnabled->SetEnabled(false);
	fForceDesktopCheckBox->SetEnabled(false);
	fZoomControl->SetEnabled(false);
	fUserAgentControl->SetEnabled(false);

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
		.Add(fForceDesktopCheckBox)
		.Add(fZoomControl)
		.Add(fUserAgentControl)
		.Add(fClearDataButton)
		.SetInsets(10, 10, 10, 10)
	);

	CenterOnScreen();
}


PermissionsWindow::~PermissionsWindow()
{
	if (fTarget.IsValid())
		fTarget.SendMessage(PERMISSIONS_WINDOW_CLOSED);

	_ClearPermissions();
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
				// Check for duplicates
				bool exists = false;
				for (int32 i = 0; i < fDomainList->CountItems(); i++) {
					PermissionItem* item = dynamic_cast<PermissionItem*>(fDomainList->ItemAt(i));
					if (item && item->Text() == domain) {
						exists = true;
						fDomainList->Select(i);
						break;
					}
				}

				if (!exists) {
					// Use default settings
					PermissionItem* item = new PermissionItem(domain, true, true, false, 1.0, false, "");
					if (fDomainList->AddItem(item)) {
						fAddDomainControl->SetText("");
						_SavePermissions();
					} else {
						delete item;
					}
				}
			}
			break;
		}

		case MSG_REMOVE_DOMAIN:
		{
			int32 selection = fDomainList->CurrentSelection();
			if (selection >= 0) {
				PermissionItem* item = dynamic_cast<PermissionItem*>(fDomainList->ItemAt(selection));
				if (item) {
					SitePermissionsManager::Instance()->RemovePermission(item->Text());
					delete fDomainList->RemoveItem(selection);
					SitePermissionsManager::Instance()->Save();
					_UpdateFields();
				}
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
					item->SetForceDesktop(fForceDesktopCheckBox->Value() == B_CONTROL_ON);
					item->SetCustomUserAgent(fUserAgentControl->Text());
					float zoom = atof(fZoomControl->Text());
					if (zoom < 0.1) zoom = 0.1;
					if (zoom > 10.0) zoom = 10.0;
					item->SetZoom(zoom);
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
	if (fQuitting)
		return true;

	if (!IsHidden()) {
		_ClearPermissions();
		Hide();
	}
	return false;
}


void
PermissionsWindow::PrepareToQuit()
{
	fQuitting = true;
}


void
PermissionsWindow::SetTarget(const BMessenger& target)
{
	fTarget = target;
}


void
PermissionsWindow::Show()
{
	_LoadPermissions();
	BWindow::Show();
}


void
PermissionsWindow::WindowActivated(bool active)
{
	if (active)
		_LoadPermissions();
	BWindow::WindowActivated(active);
}


// Helper to manage BMessage per domain
static void
SetDomainSettings(BMessage& msg, bool js, bool cookies, bool popups, float zoom, bool forceDesktop)
{
	msg.RemoveName("js");
	msg.RemoveName("cookies");
	msg.RemoveName("popups");
	msg.RemoveName("zoom");
	msg.RemoveName("forceDesktop");
	msg.AddBool("js", js);
	msg.AddBool("cookies", cookies);
	msg.AddBool("popups", popups);
	msg.AddFloat("zoom", zoom);
	msg.AddBool("forceDesktop", forceDesktop);
}

void
PermissionsWindow::_LoadPermissions()
{
	for (int32 i = fDomainList->CountItems() - 1; i >= 0; i--)
		delete fDomainList->RemoveItem(i);

	std::map<BString, SitePermissionsManager::PermissionEntry> permissions
		= SitePermissionsManager::Instance()->GetPermissions();

	std::map<BString, SitePermissionsManager::PermissionEntry>::iterator it;
	for (it = permissions.begin(); it != permissions.end(); ++it) {
		const SitePermissionsManager::PermissionEntry& entry = it->second;
		PermissionItem* item = new PermissionItem(entry.domain, entry.js,
			entry.cookies, entry.popups, entry.zoom, entry.forceDesktop,
			entry.customUserAgent);
		fDomainList->AddItem(item);
	}
}


void
PermissionsWindow::_ClearPermissions()
{
	for (int32 i = fDomainList->CountItems() - 1; i >= 0; i--)
		delete fDomainList->RemoveItem(i);
}


void
PermissionsWindow::_SavePermissions()
{
	for (int32 i = 0; i < fDomainList->CountItems(); i++) {
		PermissionItem* item = dynamic_cast<PermissionItem*>(fDomainList->ItemAt(i));
		if (!item) continue;

		SitePermissionsManager::PermissionEntry entry;
		entry.domain = item->Text();
		entry.js = item->JS();
		entry.cookies = item->Cookies();
		entry.popups = item->Popups();
		entry.zoom = item->Zoom();
		entry.forceDesktop = item->ForceDesktop();
		entry.customUserAgent = item->CustomUserAgent();

		SitePermissionsManager::Instance()->UpdatePermission(entry);
	}
	SitePermissionsManager::Instance()->Save();
	// Reload not needed since we updated the manager directly
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
	fForceDesktopCheckBox->SetEnabled(hasSelection);
	fZoomControl->SetEnabled(hasSelection);
	fUserAgentControl->SetEnabled(hasSelection);
	fClearDataButton->SetEnabled(hasSelection);

	if (hasSelection) {
		PermissionItem* item = dynamic_cast<PermissionItem*>(fDomainList->ItemAt(selection));
		if (item) {
			fJSEnabled->SetValue(item->JS() ? B_CONTROL_ON : B_CONTROL_OFF);
			fCookiesEnabled->SetValue(item->Cookies() ? B_CONTROL_ON : B_CONTROL_OFF);
			fPopupsEnabled->SetValue(item->Popups() ? B_CONTROL_ON : B_CONTROL_OFF);
			fForceDesktopCheckBox->SetValue(item->ForceDesktop() ? B_CONTROL_ON : B_CONTROL_OFF);
			fUserAgentControl->SetText(item->CustomUserAgent());
			BString zoomStr;
			zoomStr << item->Zoom();
			fZoomControl->SetText(zoomStr.String());
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
	BrowsingHistory::DefaultInstance()->RemoveItemsForDomain(domain);
}
