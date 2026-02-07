/*
 * Copyright (C) 2007 Andrea Anzani <andrea.anzani@gmail.com>
 * Copyright (C) 2007, 2010 Ryan Leavengood <leavengood@gmail.com>
 * Copyright (C) 2009 Maxime Simon <simon.maxime@gmail.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 * Copyright (C) 2010 Michael Lotz <mmlr@mlotz.ch>
 * Copyright (C) 2010 Rene Gollent <rene@gollent.com>
 * Copyright 2013-2015 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef PAGE_USER_DATA_H
#define PAGE_USER_DATA_H


#include <Bitmap.h>
#include <String.h>
#include <View.h>

#include "WebView.h"


class PageUserData : public BWebView::UserData {
public:
	PageUserData(BView* focusedView)
		:
		fFocusedView(focusedView),
		fPageIcon(NULL),
		fPageIconLarge(NULL),
		fURLInputSelectionStart(-1),
		fURLInputSelectionEnd(-1),
		fIsLoading(false),
		fIsDownloadRestart(false),
		fIsBypassingCache(false),
		fHttpsUpgraded(false),
		fIsLazy(false),
		fIsDiscarded(false),
		fPreview(NULL),
		fId(0)
	{
	}

	~PageUserData()
	{
		delete fPageIcon;
		delete fPageIconLarge;
		delete fPreview;
	}

	void SetFocusedView(BView* focusedView)
	{
		fFocusedView = focusedView;
	}

	BView* FocusedView() const
	{
		return fFocusedView;
	}

	void SetPageIcon(const BBitmap* icon)
	{
		delete fPageIcon;
		fPageIcon = NULL;
		delete fPageIconLarge;
		fPageIconLarge = NULL;

		if (icon == NULL)
			return;

		if (icon->Bounds().IntegerWidth() > 16) {
			fPageIconLarge = new BBitmap(icon);
			fPageIcon = new BBitmap(BRect(0, 0, 15, 15), B_RGBA32, true);
			if (fPageIcon->IsValid()) {
				BView* view = new BView(fPageIcon->Bounds(), "tmp",
					B_FOLLOW_NONE, B_WILL_DRAW);
				fPageIcon->AddChild(view);
				fPageIcon->Lock();
				view->SetHighColor(B_TRANSPARENT_32_BIT);
				view->FillRect(view->Bounds());
				view->SetDrawingMode(B_OP_ALPHA);
				view->DrawBitmap(icon, fPageIcon->Bounds());
				view->Sync();
				fPageIcon->Unlock();
				fPageIcon->RemoveChild(view);
				delete view;
			} else {
				delete fPageIcon;
				fPageIcon = new BBitmap(icon);
			}
		} else {
			fPageIcon = new BBitmap(icon);
		}
	}

	const BBitmap* PageIcon() const
	{
		return fPageIcon;
	}

	const BBitmap* PageIconLarge() const
	{
		return fPageIconLarge;
	}

	void SetURLInputContents(const char* text)
	{
		fURLInputContents = text;
	}

	const BString& URLInputContents() const
	{
		return fURLInputContents;
	}

	void SetURLInputSelection(int32 selectionStart, int32 selectionEnd)
	{
		fURLInputSelectionStart = selectionStart;
		fURLInputSelectionEnd = selectionEnd;
	}

	int32 URLInputSelectionStart() const
	{
		return fURLInputSelectionStart;
	}

	int32 URLInputSelectionEnd() const
	{
		return fURLInputSelectionEnd;
	}

	void SetIsLoading(bool loading)
	{
		fIsLoading = loading;
	}

	bool IsLoading() const
	{
		return fIsLoading;
	}

	void SetIsDownloadRestart(bool isDownloadRestart)
	{
		fIsDownloadRestart = isDownloadRestart;
	}

	bool IsDownloadRestart() const
	{
		return fIsDownloadRestart;
	}

	void SetIsBypassingCache(bool bypassing)
	{
		fIsBypassingCache = bypassing;
	}

	bool IsBypassingCache() const
	{
		return fIsBypassingCache;
	}

	void SetExpectedUpgradedUrl(const BString& url)
	{
		fExpectedUpgradedUrl = url;
	}

	const BString& ExpectedUpgradedUrl() const
	{
		return fExpectedUpgradedUrl;
	}

	void SetHttpsUpgraded(bool upgraded)
	{
		fHttpsUpgraded = upgraded;
	}

	bool IsHttpsUpgraded() const
	{
		return fHttpsUpgraded;
	}

	void SetPendingURL(const BString& url)
	{
		fPendingURL = url;
	}

	const BString& PendingURL() const
	{
		return fPendingURL;
	}

	void SetIsLazy(bool lazy)
	{
		fIsLazy = lazy;
	}

	bool IsLazy() const
	{
		return fIsLazy;
	}

	void SetIsDiscarded(bool discarded)
	{
		fIsDiscarded = discarded;
	}

	bool IsDiscarded() const
	{
		return fIsDiscarded;
	}

	void SetAllowedInsecureHost(const BString& host)
	{
		fAllowedInsecureHost = host;
	}

	const BString& AllowedInsecureHost() const
	{
		return fAllowedInsecureHost;
	}

	void SetPreview(const BBitmap* bitmap)
	{
		delete fPreview;
		if (bitmap)
			fPreview = new BBitmap(bitmap);
		else
			fPreview = NULL;
	}

	const BBitmap* Preview() const
	{
		return fPreview;
	}

	void SetId(uint32 id)
	{
		fId = id;
	}

	uint32 Id() const
	{
		return fId;
	}

private:
	BView*		fFocusedView;
	BBitmap*	fPageIcon;
	BBitmap*	fPageIconLarge;
	BString		fURLInputContents;
	int32		fURLInputSelectionStart;
	int32		fURLInputSelectionEnd;
	bool		fIsLoading;
	bool		fIsDownloadRestart;
	bool		fIsBypassingCache;
	BString		fExpectedUpgradedUrl;
	bool		fHttpsUpgraded;
	BString		fPendingURL;
	bool		fIsLazy;
	bool		fIsDiscarded;
	BString		fAllowedInsecureHost;
	BBitmap*	fPreview;
	uint32		fId;
};


#endif // PAGE_USER_DATA_H
