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
	fPermissionMap.clear();

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
				name.ToLower();
				PermissionEntry entry;
				entry.domain = name;
				// Defaults based on logic found in PermissionsWindow.cpp/BrowserWindow.cpp
				// Though in loading, we just read what is there.
				if (domainMsg.FindBool("js", &entry.js) != B_OK) entry.js = true;
				if (domainMsg.FindBool("cookies", &entry.cookies) != B_OK) entry.cookies = true;
				if (domainMsg.FindBool("popups", &entry.popups) != B_OK) entry.popups = false;

				fPermissionMap[name] = entry;
			}
		}
	}
}


bool
SitePermissionsManager::CheckPermission(const char* url, bool& allowJS, bool& allowCookies, bool& allowPopups)
{
	BUrl bUrl(url);
	BString host = bUrl.Host();
	host.ToLower();
	bool found = false;
	allowJS = true;
	allowCookies = true;
	allowPopups = true;

	BAutolock lock(fLock);
	BString currentHost = host;
	while (currentHost.Length() > 0) {
		std::map<BString, PermissionEntry>::iterator it = fPermissionMap.find(currentHost);
		if (it != fPermissionMap.end()) {
			const PermissionEntry& entry = it->second;
			found = true;
			allowJS = entry.js;
			allowCookies = entry.cookies;
			allowPopups = entry.popups;
			break;
		}

		int dotIndex = currentHost.FindFirst('.');
		if (dotIndex < 0)
			break;
		currentHost.Remove(0, dotIndex + 1);
	}

	return found;
}
