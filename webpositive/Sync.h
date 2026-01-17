/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYNC_H
#define SYNC_H

#include <Path.h>
#include <SupportDefs.h>

namespace BPrivate {
	namespace Network {
		class BNetworkCookieJar;
	}
}

class Sync {
public:
	static status_t ExportProfile(const BPath& folder,
		BPrivate::Network::BNetworkCookieJar& cookieJar);
	static status_t ImportProfile(const BPath& folder,
		BPrivate::Network::BNetworkCookieJar& cookieJar);

	// Helper to export cookies
	static status_t ExportCookies(const BPath& path,
		BPrivate::Network::BNetworkCookieJar& cookieJar);
	// Helper to import cookies
	static status_t ImportCookies(const BPath& path,
		BPrivate::Network::BNetworkCookieJar& cookieJar);
};

#endif // SYNC_H
