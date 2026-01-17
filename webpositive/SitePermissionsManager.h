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
#include <vector>

class SitePermissionsManager {
public:
	static SitePermissionsManager* Instance();

	bool CheckPermission(const char* url, bool& allowJS, bool& allowCookies, bool& allowPopups);
	void Reload();

private:
	SitePermissionsManager();
	~SitePermissionsManager();

	struct PermissionEntry {
		BString domain;
		bool js;
		bool cookies;
		bool popups;
	};

	std::map<BString, PermissionEntry> fPermissionMap;
	BLocker fLock;
};

#endif // SITE_PERMISSIONS_MANAGER_H
