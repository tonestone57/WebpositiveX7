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
		fHttpsUpgraded(false)
	{
	}

	~PageUserData()
	{
		delete fPageIcon;
		delete fPageIconLarge;
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

private:
	BView*		fFocusedView;
	BBitmap*	fPageIcon;
	BBitmap*	fPageIconLarge;
	BString		fURLInputContents;
	int32		fURLInputSelectionStart;
	int32		fURLInputSelectionEnd;
	bool		fIsLoading;
	bool		fIsDownloadRestart;
	BString		fExpectedUpgradedUrl;
	bool		fHttpsUpgraded;
};


#endif // PAGE_USER_DATA_H
