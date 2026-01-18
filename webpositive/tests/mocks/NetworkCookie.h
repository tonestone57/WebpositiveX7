/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NETWORK_COOKIE_H
#define _NETWORK_COOKIE_H

#include <String.h>
#include <SupportDefs.h>

namespace BPrivate {
namespace Network {

class BNetworkCookie {
public:
	BNetworkCookie(const char* name, const char* value)
		: fName(name), fValue(value), fSecure(false), fExpiration(0) {}

    // Overload for BString
    BNetworkCookie(const BString& name, const BString& value)
		: fName(name), fValue(value), fSecure(false), fExpiration(0) {}

	void SetDomain(const char* domain) { fDomain = domain; }
    void SetDomain(const BString& domain) { fDomain = domain; }

	const BString& Domain() const { return fDomain; }

	void SetPath(const char* path) { fPath = path; }
    void SetPath(const BString& path) { fPath = path; }

	const BString& Path() const { return fPath; }

	void SetSecure(bool secure) { fSecure = secure; }
	bool Secure() const { return fSecure; }

	void SetExpirationDate(time_t expiration) { fExpiration = expiration; }
	time_t ExpirationDate() const { return fExpiration; }

	const BString& Name() const { return fName; }
	const BString& Value() const { return fValue; }

private:
	BString fDomain;
	BString fPath;
	BString fName;
	BString fValue;
	bool fSecure;
	time_t fExpiration;
};

} // namespace Network
} // namespace BPrivate

#endif // _NETWORK_COOKIE_H
