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

				fPermissions.push_back(entry);
			}
		}
	}
}


bool
SitePermissionsManager::CheckPermission(const char* url, bool& allowPopups)
{
	BUrl bUrl(url);
	BString host = bUrl.Host();
	bool found = false;
	allowPopups = true; // Default behavior if not found?
	// In BrowserWindow logic:
	// bool allowPopups = true;
	// ... if found ... if (domainMsg.FindBool("popups", &popups) == B_OK) allowPopups = popups; else allowPopups = false;
	// if (found && !allowPopups) -> Block

	BAutolock lock(fLock);
	for (size_t i = 0; i < fPermissions.size(); i++) {
		const PermissionEntry& entry = fPermissions[i];
		// Existing logic: host.IFindFirst(name) >= 0
		if (host.IFindFirst(entry.domain) >= 0) {
			found = true;
			allowPopups = entry.popups;
			break;
		}
	}

	return found;
}
