/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues
 */
#ifndef COOKIE_WINDOW_H
#define COOKIE_WINDOW_H


#include <Window.h>

#include <map>
#include <vector>
#include <memory>

#include <NetworkCookieJar.h>


class BColumnListView;
class BOutlineListView;
class BStringItem;
class BStringView;
class BString;
class BFilePanel;


class CookieWindow : public BWindow {
public:
								CookieWindow(BRect frame,
									BPrivate::Network::BNetworkCookieJar& jar);
	virtual						~CookieWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual void				Show();
	virtual void				Hide();
	virtual	bool				QuitRequested();

			void				PrepareToQuit();

private:
			void				_BuildDomainList();
			void				_EmptyDomainList();
			void				_ShowCookiesForDomain(BString domain);
			void				_DeleteCookies();

private:
	BOutlineListView*			fDomains;
	BColumnListView*			fCookies;
	BStringView*				fHeaderView;

	bool						fQuitting;
	BPrivate::Network::BNetworkCookieJar&	fCookieJar;

	std::map<BString, std::vector<BPrivate::Network::BNetworkCookie> > fCookieMap;
	std::unique_ptr<BFilePanel>	fImportPanel;
	std::unique_ptr<BFilePanel>	fExportPanel;
};


#endif // COOKIE_WINDOW_H

