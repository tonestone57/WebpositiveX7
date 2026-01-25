/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "CredentialsStorage.h"

#include <new>
#include <stdio.h>

#include <Autolock.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <KeyStore.h>
#include <Message.h>
#include <Path.h>

#include "BrowserApp.h"


Credentials::Credentials()
	:
	fUsername(),
	fPassword()
{
}


Credentials::Credentials(const BString& username, const BString& password)
	:
	fUsername(username),
	fPassword(password)
{
}


Credentials::Credentials(const Credentials& other)
{
	*this = other;
}


Credentials::Credentials(const BMessage* archive)
{
	if (archive == NULL)
		return;
	archive->FindString("username", &fUsername);
	archive->FindString("password", &fPassword);
}


Credentials::~Credentials()
{
	if (fPassword.Length() > 0) {
		char* ptr = fPassword.LockBuffer(fPassword.Length());
		memset(ptr, 0, fPassword.Length());
		fPassword.UnlockBuffer(0);
	}
}


status_t
Credentials::Archive(BMessage* archive) const
{
	if (archive == NULL)
		return B_BAD_VALUE;
	status_t status = archive->AddString("username", fUsername);
	if (status == B_OK)
		status = archive->AddString("password", fPassword);
	return status;
}


Credentials&
Credentials::operator=(const Credentials& other)
{
	if (this == &other)
		return *this;

	fUsername = other.fUsername;
	// Force deep copy to avoid COW, so we can wipe the buffer later without affecting others
	// or being affected by COW mechanics (which might allocate a new buffer on LockBuffer).
	fPassword.SetTo(other.fPassword.String());

	return *this;
}


bool
Credentials::operator==(const Credentials& other) const
{
	if (this == &other)
		return true;

	return fUsername == other.fUsername && fPassword == other.fPassword;
}


bool
Credentials::operator!=(const Credentials& other) const
{
	return !(*this == other);
}


const BString&
Credentials::Username() const
{
	return fUsername;
}


const BString&
Credentials::Password() const
{
	return fPassword;
}


// #pragma mark - CredentialsStorage


CredentialsStorage
CredentialsStorage::sPersistentInstance(true);


CredentialsStorage
CredentialsStorage::sSessionInstance(false);


CredentialsStorage::CredentialsStorage(bool persistent)
	:
	BLocker(persistent ? "persistent credential storage"
		: "credential storage"),
	fCredentialMap(),
	fSettingsLoaded(false),
	fPersistent(persistent)
{
}


CredentialsStorage::~CredentialsStorage()
{
}


/*static*/ CredentialsStorage*
CredentialsStorage::SessionInstance()
{
	return &sSessionInstance;
}


/*static*/ CredentialsStorage*
CredentialsStorage::PersistentInstance()
{
	if (sPersistentInstance.Lock()) {
		sPersistentInstance._LoadSettings();
		sPersistentInstance.Unlock();
	}
	return &sPersistentInstance;
}


bool
CredentialsStorage::Contains(const HashString& key)
{
	BAutolock _(this);

	return fCredentialMap.ContainsKey(key);
}


status_t
CredentialsStorage::PutCredentials(const HashString& key,
	const Credentials& credentials)
{
	BAutolock _(this);

	status_t status = fCredentialMap.Put(key, credentials);
	if (status != B_OK)
		return status;

	if (fPersistent) {
		// BKeyStore keyStore;
		// return keyStore.SetPassword("WebPositive", key.GetString(),
		// 	credentials.Password(), credentials.Username());
	}
	return B_OK;
}


Credentials
CredentialsStorage::GetCredentials(const HashString& key)
{
	BAutolock _(this);

	return fCredentialMap.Get(key);
}


void
CredentialsStorage::RemoveCredentials(const HashString& key)
{
	BAutolock _(this);

	if (fCredentialMap.ContainsKey(key)) {
		fCredentialMap.Remove(key);
		if (fPersistent) {
			// BKeyStore keyStore;
			// keyStore.RemovePassword("WebPositive", key.GetString());
		}
	}
}


// #pragma mark - private


void
CredentialsStorage::_LoadSettings()
{
	if (!fPersistent || fSettingsLoaded)
		return;

	fSettingsLoaded = true;

	// BKeyStore keyStore;
	// BMessage passwords;
	// if (keyStore.GetKeyring("WebPositive", &passwords) == B_OK) {
	// 	BMessage passwordMsg;
	// 	for (int32 i = 0; passwords.FindMessage("password", i, &passwordMsg) == B_OK; i++) {
	// 		const char* identifier;
	// 		const char* password;
	// 		const char* secondaryInfo;
	// 		if (passwordMsg.FindString("identifier", &identifier) == B_OK
	// 			&& passwordMsg.FindString("password", &password) == B_OK
	// 			&& passwordMsg.FindString("secondaryInfo", &secondaryInfo) == B_OK) {
	//
	// 			Credentials credentials(secondaryInfo, password);
	// 			fCredentialMap.Put(identifier, credentials);
	// 		}
	// 	}
	// }

	// Migration from legacy flat file
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK
		&& path.Append(kApplicationName) == B_OK
		&& path.Append("CredentialsStorage") == B_OK) {

		BFile file(path.Path(), B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			BMessage settingsArchive;
			if (settingsArchive.Unflatten(&file) == B_OK) {
				BMessage credentialsArchive;
				for (int32 i = 0; settingsArchive.FindMessage("credentials", i, &credentialsArchive) == B_OK; i++) {
					BString key;
					if (credentialsArchive.FindString("key", &key) == B_OK) {
						Credentials credentials(&credentialsArchive);
						PutCredentials(key.String(), credentials);
					}
				}
			}
			// Remove legacy file after successful migration attempt
			BEntry entry(path.Path());
			entry.Remove();
		}
	}
}
