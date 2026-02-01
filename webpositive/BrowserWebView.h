/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef BROWSER_WEB_VIEW_H
#define BROWSER_WEB_VIEW_H

#include "WebView.h"

namespace BPrivate {
namespace Network {
	class BUrlContext;
}
}

class BrowserWebView : public BWebView {
public:
	BrowserWebView(const char* name, BPrivate::Network::BUrlContext* context);
	virtual ~BrowserWebView();

	virtual void MessageReceived(BMessage* message);
	void SetZoomTextOnly(bool zoomTextOnly);

private:
	bool fZoomTextOnly;
};

#endif // BROWSER_WEB_VIEW_H
