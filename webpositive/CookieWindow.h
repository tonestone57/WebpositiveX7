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

#include <NetworkCookieJar.h>


class BColumnListView;
class BOutlineListView;
class BStringItem;
class BStringView;
class BString;


class CookieWindow : public BWindow {
public:
								CookieWindow(BRect frame,
									BPrivate::Network::BNetworkCookieJar& jar);
	virtual	void				MessageReceived(BMessage* message);
	virtual void				Show();
	virtual void				Hide();
	virtual	bool				QuitRequested();

private:
			void				_BuildDomainList();
			void				_EmptyDomainList();
			void				_ShowCookiesForDomain(BString domain);
			void				_DeleteCookies();

private:
	BOutlineListView*			fDomains;
	BColumnListView*			fCookies;
	BStringView*				fHeaderView;

	BPrivate::Network::BNetworkCookieJar&	fCookieJar;

	std::map<BString, std::vector<BPrivate::Network::BNetworkCookie> > fCookieMap;
};


#endif // COOKIE_WINDOW_H

