/*
 * Copyright 2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SITE_PERMISSIONS_MANAGER_H
#define SITE_PERMISSIONS_MANAGER_H

#include <SupportDefs.h>
#include <String.h>
#include <Locker.h>
#include <map>

class SitePermissionsManager {
public:
	static SitePermissionsManager* Instance();

	bool CheckPermission(const char* url, bool& allowJS, bool& allowCookies, bool& allowPopups, float& zoom, bool& forceDesktop);
	void Reload();
	void SetZoom(const char* domain, float zoom);

	struct PermissionEntry {
		BString domain;
		bool js;
		bool cookies;
		bool popups;
		float zoom;
		bool forceDesktop;
		BString customUserAgent;
	};

	void UpdatePermission(const PermissionEntry& entry);
	void Save();

private:
	SitePermissionsManager();
	~SitePermissionsManager();

	void _Save();

	std::map<BString, PermissionEntry> fPermissionMap;
	BLocker fLock;
};

#endif // SITE_PERMISSIONS_MANAGER_H
