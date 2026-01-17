/*
 * Copyright 2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "SitePermissionsManager.h"

#include <Autolock.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Url.h>
#include <SettingsMessage.h>

extern const char* kApplicationName;

SitePermissionsManager* SitePermissionsManager::sInstance = NULL;


SitePermissionsManager*
SitePermissionsManager::Instance()
{
	if (sInstance == NULL)
		sInstance = new SitePermissionsManager();
	return sInstance;
}


SitePermissionsManager::SitePermissionsManager()
	:
	fLock("SitePermissionsManager Lock")
{
	Reload();
}


SitePermissionsManager::~SitePermissionsManager()
{
}


void
SitePermissionsManager::Reload()
{
	BAutolock lock(fLock);
	fPermissions.clear();

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(kApplicationName);
		path.Append("SitePermissions");
		SettingsMessage settings(B_USER_SETTINGS_DIRECTORY, path.Path());

		int32 i = 0;
		BMessage domainMsg;
		while (settings.FindMessage("domain", i++, &domainMsg) == B_OK) {
			BString name;
			if (domainMsg.FindString("name", &name) == B_OK) {
				PermissionEntry entry;
				entry.domain = name;
				// Defaults based on logic found in PermissionsWindow.cpp/BrowserWindow.cpp
				// Though in loading, we just read what is there.
				if (domainMsg.FindBool("js", &entry.js) != B_OK) entry.js = true;
				if (domainMsg.FindBool("cookies", &entry.cookies) != B_OK) entry.cookies = true;
				if (domainMsg.FindBool("popups", &entry.popups) != B_OK) entry.popups = false;
				if (domainMsg.FindFloat("zoom", &entry.zoom) != B_OK) entry.zoom = 1.0;
				if (domainMsg.FindBool("forceDesktop", &entry.forceDesktop) != B_OK) entry.forceDesktop = false;

				fPermissions.push_back(entry);
			}
		}
	}
}


bool
SitePermissionsManager::CheckPermission(const char* url, bool& allowJS, bool& allowCookies, bool& allowPopups, float& zoom, bool& forceDesktop)
{
	BUrl bUrl(url);
	BString host = bUrl.Host();
	bool found = false;
	allowJS = true;
	allowCookies = true;
	allowPopups = true;
	zoom = 1.0;
	forceDesktop = false;

	BAutolock lock(fLock);
	for (size_t i = 0; i < fPermissions.size(); i++) {
		const PermissionEntry& entry = fPermissions[i];
		if (host == entry.domain || host.EndsWith(BString(".") << entry.domain)) {
			found = true;
			allowJS = entry.js;
			allowCookies = entry.cookies;
			allowPopups = entry.popups;
			zoom = entry.zoom;
			forceDesktop = entry.forceDesktop;
			break;
		}
	}

	return found;
}

void
SitePermissionsManager::SetZoom(const char* domain, float zoom)
{
	BAutolock lock(fLock);
	bool found = false;
	for (size_t i = 0; i < fPermissions.size(); i++) {
		if (fPermissions[i].domain == domain) {
			fPermissions[i].zoom = zoom;
			found = true;
			break;
		}
	}

	if (!found) {
		PermissionEntry entry;
		entry.domain = domain;
		entry.js = true;
		entry.cookies = true;
		entry.popups = false;
		entry.forceDesktop = false;
		entry.zoom = zoom;
		fPermissions.push_back(entry);
	}

	// Release lock before saving to avoid potential issues if Save takes time
	// though Save currently just writes a file.
	// But Save() uses the lock, so we must be careful.
	// Actually Save() re-acquires lock if we call it.
	// So we should NOT hold lock when calling Save().
	lock.Unlock();
	Save();
}

void
SitePermissionsManager::UpdatePermission(const PermissionEntry& entry)
{
	BAutolock lock(fLock);
	bool found = false;
	for (size_t i = 0; i < fPermissions.size(); i++) {
		if (fPermissions[i].domain == entry.domain) {
			fPermissions[i] = entry;
			found = true;
			break;
		}
	}
	if (!found) {
		fPermissions.push_back(entry);
	}
}

void
SitePermissionsManager::Save()
{
	BAutolock lock(fLock);
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(kApplicationName);
		path.Append("SitePermissions");
		SettingsMessage settings(B_USER_SETTINGS_DIRECTORY, path.Path());
		settings.MakeEmpty();

		for (size_t i = 0; i < fPermissions.size(); i++) {
			BMessage domainMsg;
			domainMsg.AddString("name", fPermissions[i].domain);
			domainMsg.AddBool("js", fPermissions[i].js);
			domainMsg.AddBool("cookies", fPermissions[i].cookies);
			domainMsg.AddBool("popups", fPermissions[i].popups);
			domainMsg.AddFloat("zoom", fPermissions[i].zoom);
			domainMsg.AddBool("forceDesktop", fPermissions[i].forceDesktop);

			settings.AddMessage("domain", &domainMsg);
		}
		settings.Save();
	}
}
