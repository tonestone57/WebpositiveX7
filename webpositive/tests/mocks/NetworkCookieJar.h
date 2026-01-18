/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NETWORK_COOKIE_JAR_H
#define _NETWORK_COOKIE_JAR_H

#include <NetworkCookie.h>
#include <vector>

namespace BPrivate {
namespace Network {

class BNetworkCookieJar {
public:
	BNetworkCookieJar() {}

	void AddCookie(const BNetworkCookie& cookie) {
		fCookies.push_back(cookie);
	}

	class Iterator {
	public:
		Iterator(const std::vector<BNetworkCookie>& cookies)
			: fCookies(cookies), fIndex(0) {}

		const BNetworkCookie* Next() {
			if (fIndex < fCookies.size()) {
				return &fCookies[fIndex++];
			}
			return NULL;
		}

	private:
		const std::vector<BNetworkCookie>& fCookies;
		size_t fIndex;
	};

	Iterator GetIterator() const {
		return Iterator(fCookies);
	}

	// Helper for testing
	const std::vector<BNetworkCookie>& GetCookies() const { return fCookies; }
    void MakeEmpty() { fCookies.clear(); }

private:
	std::vector<BNetworkCookie> fCookies;
};

} // namespace Network
} // namespace BPrivate

#endif // _NETWORK_COOKIE_JAR_H
