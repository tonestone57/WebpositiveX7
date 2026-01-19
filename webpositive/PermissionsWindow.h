/*
 * Copyright 2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef PERMISSIONS_WINDOW_H
#define PERMISSIONS_WINDOW_H

#include <Window.h>
#include <Messenger.h>
#include <NetworkCookieJar.h>

class BListView;
class BButton;
class BCheckBox;
class BTextControl;

namespace BPrivate {
namespace Network {
	class BNetworkCookieJar;
}
}

class PermissionsWindow : public BWindow {
public:
								PermissionsWindow(BRect frame, BPrivate::Network::BNetworkCookieJar& jar);
	virtual						~PermissionsWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual void				WindowActivated(bool active);
	virtual	bool				QuitRequested();
	virtual void				Show();

			void				SetTarget(const BMessenger& target);
			void				PrepareToQuit();
private:
			void				_LoadPermissions();
			void				_ClearPermissions();
			void				_SavePermissions();
			void				_UpdateFields();
			void				_ClearSiteData(const char* domain);

private:
			BPrivate::Network::BNetworkCookieJar& fCookieJar;
			BMessenger			fTarget;

			bool				fQuitting;
			BListView*			fDomainList;
			BCheckBox*			fJSEnabled;
			BCheckBox*			fCookiesEnabled;
			BCheckBox*			fPopupsEnabled;
			BCheckBox*			fForceDesktopCheckBox;
			BTextControl*		fZoomControl;
			BTextControl*		fUserAgentControl;
			BTextControl*		fAddDomainControl;
			BButton*			fAddButton;
			BButton*			fRemoveButton;
			BButton*			fClearDataButton;
};

#endif // PERMISSIONS_WINDOW_H
