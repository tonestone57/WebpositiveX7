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


SitePermissionsManager* SitePermissionsManager::Instance()
{
	static SitePermissionsManager* sInstance = new SitePermissionsManager();
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

				if (domainMsg.FindBool("js", &entry.js) != B_OK) entry.js = true;
				if (domainMsg.FindBool("cookies", &entry.cookies) != B_OK) entry.cookies = true;
				if (domainMsg.FindBool("popups", &entry.popups) != B_OK) entry.popups = false;
				if (domainMsg.FindFloat("zoom", &entry.zoom) != B_OK) entry.zoom = 1.0;
				if (domainMsg.FindBool("forceDesktop", &entry.forceDesktop) != B_OK) entry.forceDesktop = false;

				fPermissionMap[name] = entry;
			}
		}
	}
}


bool
SitePermissionsManager::CheckPermission(const char* url, bool& allowJS, bool& allowCookies, bool& allowPopups, float& zoom, bool& forceDesktop)
{
	BUrl bUrl(url);
	BString host = bUrl.Host();
	host.ToLower();
	bool found = false;
	allowJS = true;
	allowCookies = true;
	allowPopups = true;
	zoom = 1.0;
	forceDesktop = false;

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
			zoom = entry.zoom;
			forceDesktop = entry.forceDesktop;
			break;
		}

		int dotIndex = currentHost.FindFirst('.');
		if (dotIndex < 0)
			break;
		currentHost.Remove(0, dotIndex + 1);
	}

	return found;
}

void
SitePermissionsManager::SetZoom(const char* domain, float zoom)
{
	BAutolock lock(fLock);
	BString host(domain);
	host.ToLower();

	std::map<BString, PermissionEntry>::iterator it = fPermissionMap.find(host);
	if (it != fPermissionMap.end()) {
		it->second.zoom = zoom;
	} else {
		PermissionEntry entry;
		entry.domain = host;
		entry.js = true;
		entry.cookies = true;
		entry.popups = false;
		entry.forceDesktop = false;
		entry.zoom = zoom;
		fPermissionMap[host] = entry;
	}

	// Release lock before saving
	lock.Unlock();
	Save();
}

void
SitePermissionsManager::UpdatePermission(const PermissionEntry& entry)
{
	BAutolock lock(fLock);
	BString host(entry.domain);
	host.ToLower();
	fPermissionMap[host] = entry;
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

		std::map<BString, PermissionEntry>::iterator it;
		for (it = fPermissionMap.begin(); it != fPermissionMap.end(); ++it) {
			BMessage domainMsg;
			// Ensure we pass const char*
			domainMsg.AddString("name", it->second.domain.String());
			domainMsg.AddBool("js", it->second.js);
			domainMsg.AddBool("cookies", it->second.cookies);
			domainMsg.AddBool("popups", it->second.popups);
			domainMsg.AddFloat("zoom", it->second.zoom);
			domainMsg.AddBool("forceDesktop", it->second.forceDesktop);

			settings.AddMessage("domain", &domainMsg);
		}
		settings.Save();
	}
}
