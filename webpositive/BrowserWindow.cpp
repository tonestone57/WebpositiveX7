/*
 * Copyright (C) 2007 Andrea Anzani <andrea.anzani@gmail.com>
 * Copyright (C) 2007, 2010 Ryan Leavengood <leavengood@gmail.com>
 * Copyright (C) 2009 Maxime Simon <simon.maxime@gmail.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 * Copyright (C) 2010 Michael Lotz <mmlr@mlotz.ch>
 * Copyright (C) 2010 Rene Gollent <rene@gollent.com>
 * Copyright 2013-2015 Haiku, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "BrowserWindow.h"

#include <stdlib.h>
#include <string.h>

#include <OS.h>
#include <Alert.h>
#include <TranslatorFormats.h>
#include <TranslationUtils.h>
#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <Catalog.h>
#include <View.h>
#include <CheckBox.h>
#include <Clipboard.h>
#include <ControlLook.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <IconMenuItem.h>
#include <Keymap.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <ObjectList.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <SeparatorView.h>
#include <Size.h>
#include <SpaceLayoutItem.h>
#include <StatusBar.h>
#include <StringView.h>
#include <TextControl.h>
#include <UnicodeChar.h>
#include <Url.h>

#include <map>
#include <vector>
#include <algorithm>
#include <stdio.h>

#include "AuthenticationPanel.h"
#include "BaseURL.h"
#include "BitmapButton.h"
#include "BookmarkBar.h"
#include "BrowserApp.h"
#include "BrowsingHistory.h"
#include "CredentialsStorage.h"
#include "IconButton.h"
#include "NavMenu.h"
#include "PageUserData.h"
#include "PermissionsWindow.h"
#include "NetworkWindow.h"
#include "SettingsKeys.h"
#include "SettingsMessage.h"
#include "SitePermissionsManager.h"
#include "Sync.h"
#include "TabManager.h"
#include "TabSearchWindow.h"
#include "URLInputGroup.h"
#include "WebPage.h"
#include "WebSettings.h"
#include "WebView.h"
#include "WebViewConstants.h"
#include "WindowIcon.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "WebPositive Window"


enum {
	OPEN_LOCATION								= 'open',
	SAVE_PAGE									= 'save',
	GO_BACK										= 'goba',
	GO_FORWARD									= 'gofo',
	STOP										= 'stop',
	HOME										= 'home',
	GOTO_URL									= 'goul',
	CLEAR_HISTORY								= 'clhs',

	CREATE_BOOKMARK								= 'crbm',
	SHOW_BOOKMARKS								= 'shbm',

	ZOOM_FACTOR_INCREASE						= 'zfin',
	ZOOM_FACTOR_DECREASE						= 'zfdc',
	ZOOM_FACTOR_RESET							= 'zfrs',
	ZOOM_TEXT_ONLY								= 'zfto',

	TOGGLE_FULLSCREEN							= 'tgfs',
	TOGGLE_AUTO_HIDE_INTERFACE_IN_FULLSCREEN	= 'tgah',
	CHECK_AUTO_HIDE_INTERFACE					= 'cahi',

	SHOW_PAGE_SOURCE							= 'spgs',

	EDIT_SHOW_FIND_GROUP						= 'sfnd',
	EDIT_HIDE_FIND_GROUP						= 'hfnd',
	EDIT_FIND_NEXT								= 'fndn',
	EDIT_FIND_PREVIOUS							= 'fndp',
	FIND_TEXT_CHANGED							= 'ftxt',

	SELECT_TAB									= 'sltb',
	CYCLE_TABS									= 'ctab',

	REOPEN_CLOSED_TAB							= 'roct',
	COPY_AS_MARKDOWN							= 'cpmd',
	COPY_AS_HTML								= 'cpht',
	COPY_AS_PLAIN_TEXT							= 'cppt',

	EXPORT_COOKIES								= 'exck'
};


static BString
EscapeHTML(const BString& text)
{
	BString result(text);
	result.ReplaceAll("&", "&amp;");
	result.ReplaceAll("<", "&lt;");
	result.ReplaceAll(">", "&gt;");
	result.ReplaceAll("\"", "&quot;");
	return result;
}


static BString
EscapeMarkdown(const BString& text)
{
	BString result(text);
	// Escape characters that might break the link syntax [Title](URL)
	result.ReplaceAll("[", "\\[");
	result.ReplaceAll("]", "\\]");
	return result;
}

static BString
EscapeMarkdownURL(const BString& text)
{
	BString result(text);
	// Escape closing parenthesis in URL to avoid breaking [Title](URL)
	result.ReplaceAll(")", "%29");
	return result;
}


static const int32 kModifiers = B_SHIFT_KEY | B_COMMAND_KEY
	| B_CONTROL_KEY | B_OPTION_KEY | B_MENU_KEY;


static const char* kBookmarkBarSubdir = "Bookmark bar";


struct SyncParams {
	BPath path;
	BPrivate::Network::BUrlContext* context;
	BMessenger target;
};


struct FaviconSaveParams {
	BPath path;
	BBitmap* icon;
};


static status_t
_SaveFaviconThread(void* data)
{
	FaviconSaveParams* params = static_cast<FaviconSaveParams*>(data);

	BPath tempPath(params->path);
	BString tempFileName(tempPath.Leaf());
	tempFileName << ".tmp";
	tempPath.GetParent(&tempPath);
	tempPath.Append(tempFileName);

	BFile file(tempPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() == B_OK) {
		int32 width = params->icon->Bounds().IntegerWidth() + 1;
		int32 height = params->icon->Bounds().IntegerHeight() + 1;
		file.Write(&width, sizeof(width));
		file.Write(&height, sizeof(height));

		int32 bytesPerRow = params->icon->BytesPerRow();
		int32 rowLen = width * 4;
		uint8* bits = (uint8*)params->icon->Bits();
		for (int32 i = 0; i < height; i++)
			file.Write(bits + (i * bytesPerRow), rowLen);

		file.Unset();
		BEntry entry(tempPath.Path());
		entry.Rename(params->path.Leaf(), true);
	}

	delete params->icon;
	delete params;
	return B_OK;
}


static status_t
_ExportProfileThread(void* data)
{
	SyncParams* params = static_cast<SyncParams*>(data);
	status_t status = Sync::ExportProfile(params->path, params->context->GetCookieJar());

	if (status != B_OK) {
		BString errorMsg(B_TRANSLATE("Failed to export profile"));
		errorMsg << ": " << strerror(status);
		BAlert* alert = new BAlert(B_TRANSLATE("Export error"),
			errorMsg.String(), B_TRANSLATE("OK"));
		alert->Go();
	}

	params->context->Release();
	delete params;
	return B_OK;
}


static status_t
_ImportProfileThread(void* data)
{
	SyncParams* params = static_cast<SyncParams*>(data);
	status_t status = Sync::ImportProfile(params->path, params->context->GetCookieJar());

	if (status != B_OK) {
		BString errorMsg(B_TRANSLATE("Failed to import profile"));
		errorMsg << ": " << strerror(status);
		BAlert* alert = new BAlert(B_TRANSLATE("Import error"),
			errorMsg.String(), B_TRANSLATE("OK"));
		alert->Go();
	}

	params->context->Release();
	delete params;
	return B_OK;
}


static BLayoutItem*
layoutItemFor(BView* view)
{
	BLayout* layout = view->Parent()->GetLayout();
	int32 index = layout->IndexOfView(view);
	return layout->ItemAt(index);
}


class BookmarkMenu : public BNavMenu {
public:
	BookmarkMenu(const char* title, BHandler* target, const entry_ref* navDir)
		:
		BNavMenu(title, B_REFS_RECEIVED, target)
	{
		// Add these items here already, so the shortcuts work even when
		// the menu has never been opened yet.
		_AddStaticItems();

		SetNavDir(navDir);
	}

	virtual void AttachedToWindow()
	{
		RemoveItems(0, CountItems(), true);
		ForceRebuild();
		BNavMenu::AttachedToWindow();
		if (CountItems() > 0)
			AddItem(new BSeparatorItem(), 0);
		_AddStaticItems();
		DoLayout();
	}

private:
	void _AddStaticItems()
	{
		AddItem(new BMenuItem(B_TRANSLATE("Manage bookmarks"),
			new BMessage(SHOW_BOOKMARKS), 'M'), 0);
		AddItem(new BMenuItem(B_TRANSLATE("Bookmark this page"),
			new BMessage(CREATE_BOOKMARK), 'B'), 0);
	}
};




class CloseButton : public BButton {
public:
	CloseButton(BMessage* message)
		:
		BButton("close button", NULL, message),
		fOverCloseRect(false)
	{
		// Button is 16x16 regardless of font size
		SetExplicitMinSize(BSize(15, 15));
		SetExplicitMaxSize(BSize(15, 15));
	}

	virtual void Draw(BRect updateRect)
	{
		BRect frame = Bounds();
		BRect closeRect(frame.InsetByCopy(4, 4));
		rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
		float tint = B_DARKEN_1_TINT;

		if (fOverCloseRect)
			tint *= 1.4;
		else
			tint *= 1.2;

		if (Value() == B_CONTROL_ON && fOverCloseRect) {
			// Draw the button frame
			be_control_look->DrawButtonFrame(this, frame, updateRect,
				base, base, BControlLook::B_ACTIVATED
					| BControlLook::B_BLEND_FRAME);
			be_control_look->DrawButtonBackground(this, frame,
				updateRect, base, BControlLook::B_ACTIVATED);
			closeRect.OffsetBy(1, 1);
			tint *= 1.2;
		} else {
			SetHighColor(base);
			FillRect(updateRect);
		}

		// Draw the ×
		base = tint_color(base, tint);
		SetHighColor(base);
		SetPenSize(2);
		StrokeLine(closeRect.LeftTop(), closeRect.RightBottom());
		StrokeLine(closeRect.LeftBottom(), closeRect.RightTop());
		SetPenSize(1);
	}

	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage* dragMessage)
	{
		switch (transit) {
			case B_ENTERED_VIEW:
				fOverCloseRect = true;
				Invalidate();
				break;
			case B_EXITED_VIEW:
				fOverCloseRect = false;
				Invalidate();
				break;
			case B_INSIDE_VIEW:
				fOverCloseRect = true;
				break;
			case B_OUTSIDE_VIEW:
				fOverCloseRect = false;
				break;
		}

		BButton::MouseMoved(where, transit, dragMessage);
	}

private:
	bool fOverCloseRect;
};


// #pragma mark - BrowserWindow


BrowserWindow::BrowserWindow(BRect frame, SettingsMessage* appSettings, const BString& url,
	BPrivate::Network::BUrlContext* context, uint32 interfaceElements, BWebView* webView,
	uint32 workspaces, bool privateWindow)
	:
	BWebWindow(frame, kApplicationName, B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS, workspaces),
	fIsFullscreen(false),
	fInterfaceVisible(false),
	fMenusRunning(false),
	fVisibleInterfaceElements(interfaceElements),
	fContext(context),
	fAppSettings(appSettings),
	fZoomTextOnly(false),
	fShowTabsIfSinglePageOpen(true),
	fAutoHideInterfaceInFullscreenMode(false),
	fAutoHidePointer(false),
	fAutoHideBookmarkBar(false),
	fBookmarkBar(NULL),
	fDarkMode(false),
	fReaderMode(false),
	fToolbarBottom(false),
	fIsLoading(false),
	fLowRAMMode(false),
	fTabSearchWindow(NULL),
	fLastHistoryGeneration(0),
	fPermissionsWindow(NULL),
	fNetworkWindow(NULL),
	fIsBypassingCache(false),
	fIsPrivate(privateWindow),
	fButtonResetRunner(NULL),
	fExpectingDomInspection(false)
{
	fFormSafetyHelper.reset(new FormSafetyHelper(this));

	// Begin listening to settings changes and read some current values.
	fAppSettings->AddListener(BMessenger(this));
	fZoomTextOnly = fAppSettings->GetValue("zoom text only", fZoomTextOnly);
	fShowTabsIfSinglePageOpen = fAppSettings->GetValue(
		kSettingsKeyShowTabsIfSinglePageOpen, fShowTabsIfSinglePageOpen);

	fAutoHidePointer = fAppSettings->GetValue(kSettingsKeyAutoHidePointer,
		fAutoHidePointer);
	fAutoHideBookmarkBar = fAppSettings->GetValue(kSettingsKeyAutoHideBookmarkBar,
		fAutoHideBookmarkBar);
	fToolbarBottom = fAppSettings->GetValue(kSettingsKeyToolbarBottom,
		fToolbarBottom);
	fLowRAMMode = fAppSettings->GetValue(kSettingsKeyLowRAMMode, false);

	fNewWindowPolicy = fAppSettings->GetValue(kSettingsKeyNewWindowPolicy,
		(uint32)OpenStartPage);
	fNewTabPolicy = fAppSettings->GetValue(kSettingsKeyNewTabPolicy,
		(uint32)OpenBlankPage);
	fStartPageURL = fAppSettings->GetValue(kSettingsKeyStartPageURL,
		kDefaultStartPageURL);
	fSearchPageURL = fAppSettings->GetValue(kSettingsKeySearchPageURL,
		kDefaultSearchPageURL);

	// Create the interface elements
	BMessage* newTabMessage = new BMessage(NEW_TAB);
	newTabMessage->AddString("url", "");
	newTabMessage->AddPointer("window", this);
	newTabMessage->AddBool("select", true);
	fTabManager.reset(new TabManager(BMessenger(this), newTabMessage));

	// Menu
#if INTEGRATE_MENU_INTO_TAB_BAR
	BMenu* mainMenu = new BMenu("≡");
#else
	BMenu* mainMenu = new BMenuBar("Main menu");
#endif
	BMenu* menu = new BMenu(B_TRANSLATE("Window"));
	BMessage* newWindowMessage = new BMessage(NEW_WINDOW);
	newWindowMessage->AddString("url", "");
	BMenuItem* newItem = new BMenuItem(B_TRANSLATE("New window"),
		newWindowMessage, 'N');
	menu->AddItem(newItem);
	newItem->SetTarget(be_app);
	newItem = new BMenuItem(B_TRANSLATE("New tab"),
		new BMessage(*newTabMessage), 'T');
	menu->AddItem(newItem);
	newItem->SetTarget(be_app);

	BMessage* newPrivateWindowMessage = new BMessage(NEW_WINDOW);
	newPrivateWindowMessage->AddString("url", "");
	newPrivateWindowMessage->AddBool("private", true);
	newItem = new BMenuItem(B_TRANSLATE("New private window"),
		newPrivateWindowMessage, 'N', B_SHIFT_KEY);
	menu->AddItem(newItem);
	newItem->SetTarget(be_app);

	menu->AddItem(new BMenuItem(B_TRANSLATE("Open location"),
		new BMessage(OPEN_LOCATION), 'L'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Close window"),
		new BMessage(B_QUIT_REQUESTED), 'W', B_SHIFT_KEY));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Close tab"),
		new BMessage(CLOSE_TAB), 'W'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Save page as" B_UTF8_ELLIPSIS),
		new BMessage(SAVE_PAGE), 'S'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Import bookmarks" B_UTF8_ELLIPSIS),
		new BMessage(IMPORT_BOOKMARKS)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Export bookmarks" B_UTF8_ELLIPSIS),
		new BMessage(EXPORT_BOOKMARKS)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Import profile" B_UTF8_ELLIPSIS),
		new BMessage(SYNC_IMPORT)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Export profile" B_UTF8_ELLIPSIS),
		new BMessage(SYNC_EXPORT)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Export cookies" B_UTF8_ELLIPSIS),
		new BMessage(EXPORT_COOKIES)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Downloads"),
		new BMessage(SHOW_DOWNLOAD_WINDOW), 'D'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Settings"),
		new BMessage(SHOW_SETTINGS_WINDOW), ','));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Site permissions"),
		new BMessage(SHOW_PERMISSIONS_WINDOW)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Cookie manager"),
		new BMessage(SHOW_COOKIE_WINDOW)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Certificate info"),
		new BMessage(SHOW_CERTIFICATE_WINDOW)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Script console"),
		new BMessage(SHOW_CONSOLE_WINDOW)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Network inspector"),
		new BMessage(SHOW_NETWORK_WINDOW)));
	BMenuItem* aboutItem = new BMenuItem(B_TRANSLATE("About"),
		new BMessage(B_ABOUT_REQUESTED));
	menu->AddItem(aboutItem);
	aboutItem->SetTarget(be_app);

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Search tabs"),
		new BMessage(SEARCH_TABS), 'F', B_OPTION_KEY));
	menu->AddSeparatorItem();
	BMenuItem* quitItem = new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED), 'Q');
	menu->AddItem(quitItem);
	quitItem->SetTarget(be_app);
	mainMenu->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Edit"));
	menu->AddItem(fCutMenuItem = new BMenuItem(B_TRANSLATE("Cut"),
		new BMessage(B_CUT), 'X'));
	menu->AddItem(fCopyMenuItem = new BMenuItem(B_TRANSLATE("Copy"),
		new BMessage(B_COPY), 'C'));
	menu->AddItem(fPasteMenuItem = new BMenuItem(B_TRANSLATE("Paste"),
		new BMessage(B_PASTE), 'V'));

	BMenu* copyAsMenu = new BMenu(B_TRANSLATE("Copy page as" B_UTF8_ELLIPSIS));
	copyAsMenu->AddItem(new BMenuItem(B_TRANSLATE("Markdown"),
		new BMessage(COPY_AS_MARKDOWN)));
	copyAsMenu->AddItem(new BMenuItem(B_TRANSLATE("HTML"),
		new BMessage(COPY_AS_HTML)));
	copyAsMenu->AddItem(new BMenuItem(B_TRANSLATE("Plain text"),
		new BMessage(COPY_AS_PLAIN_TEXT)));
	menu->AddItem(copyAsMenu);

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Find"),
		new BMessage(EDIT_SHOW_FIND_GROUP), 'F'));
	menu->AddItem(fFindPreviousMenuItem
		= new BMenuItem(B_TRANSLATE("Find previous"),
		new BMessage(EDIT_FIND_PREVIOUS), 'G', B_SHIFT_KEY));
	menu->AddItem(fFindNextMenuItem = new BMenuItem(B_TRANSLATE("Find next"),
		new BMessage(EDIT_FIND_NEXT), 'G'));
	mainMenu->AddItem(menu);
	fFindPreviousMenuItem->SetEnabled(false);
	fFindNextMenuItem->SetEnabled(false);

	menu = new BMenu(B_TRANSLATE("View"));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Reload"), new BMessage(RELOAD),
		'R'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Force Reload (no cache)"), new BMessage(RELOAD_BYPASS_CACHE),
		'R', B_SHIFT_KEY));
	// the label will be replaced with the appropriate text later on
	fBookmarkBarMenuItem = new BMenuItem(B_TRANSLATE("Show bookmark bar"),
		new BMessage(SHOW_HIDE_BOOKMARK_BAR));
	menu->AddItem(fBookmarkBarMenuItem);
	fAutoHideBookmarkBarMenuItem = new BMenuItem(B_TRANSLATE("Auto-hide bookmark bar"),
		new BMessage(TOGGLE_AUTO_HIDE_BOOKMARK_BAR));
	fAutoHideBookmarkBarMenuItem->SetMarked(fAutoHideBookmarkBar);
	menu->AddItem(fAutoHideBookmarkBarMenuItem);
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Increase size"),
		new BMessage(ZOOM_FACTOR_INCREASE), '+'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Decrease size"),
		new BMessage(ZOOM_FACTOR_DECREASE), '-'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Reset size"),
		new BMessage(ZOOM_FACTOR_RESET), '0'));
	fZoomTextOnlyMenuItem = new BMenuItem(B_TRANSLATE("Zoom text only"),
		new BMessage(ZOOM_TEXT_ONLY));
	fZoomTextOnlyMenuItem->SetMarked(fZoomTextOnly);
	menu->AddItem(fZoomTextOnlyMenuItem);

	fDarkModeMenuItem = new BMenuItem(B_TRANSLATE("Dark mode"),
		new BMessage(TOGGLE_DARK_MODE));
	fDarkModeMenuItem->SetMarked(fDarkMode);
	menu->AddItem(fDarkModeMenuItem);

	fReaderModeMenuItem = new BMenuItem(B_TRANSLATE("Reader mode"),
		new BMessage(TOGGLE_READER_MODE));
	fReaderModeMenuItem->SetMarked(fReaderMode);
	menu->AddItem(fReaderModeMenuItem);

	fToolbarBottomMenuItem = new BMenuItem(B_TRANSLATE("Toolbar on bottom"),
		new BMessage(TOGGLE_TOOLBAR_BOTTOM));
	fToolbarBottomMenuItem->SetMarked(fToolbarBottom);
	menu->AddItem(fToolbarBottomMenuItem);

	fLoadImagesMenuItem = new BMenuItem(B_TRANSLATE("Disable images"),
		new BMessage(TOGGLE_LOAD_IMAGES));
	fLoadImagesMenuItem->SetMarked(!fAppSettings->GetValue(kSettingsKeyLoadImages, true));
	menu->AddItem(fLoadImagesMenuItem);

	menu->AddItem(new BMenuItem(B_TRANSLATE("Inspect Element"),
		new BMessage(INSPECT_ELEMENT)));

	menu->AddSeparatorItem();
	fFullscreenItem = new BMenuItem(B_TRANSLATE("Full screen"),
		new BMessage(TOGGLE_FULLSCREEN), B_RETURN);
	menu->AddItem(fFullscreenItem);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Page source"),
		new BMessage(SHOW_PAGE_SOURCE), 'U'));
	mainMenu->AddItem(menu);

	fHistoryMenu = new BMenu(B_TRANSLATE("History"));
	fHistoryMenu->AddItem(fBackMenuItem = new BMenuItem(B_TRANSLATE("Back"),
		new BMessage(GO_BACK), B_LEFT_ARROW));
	fHistoryMenu->AddItem(fForwardMenuItem
		= new BMenuItem(B_TRANSLATE("Forward"), new BMessage(GO_FORWARD),
		B_RIGHT_ARROW));
	fHistoryMenu->AddItem(fReopenClosedTabMenuItem = new BMenuItem(
		B_TRANSLATE("Reopen closed tab"), new BMessage(REOPEN_CLOSED_TAB), 'T',
		B_SHIFT_KEY));
	fReopenClosedTabMenuItem->SetEnabled(false);
	fHistoryMenu->AddItem(fRecentlyClosedMenu = new BMenu(
		B_TRANSLATE("Recently closed tabs")));
	fRecentlyClosedMenu->SetEnabled(false);
	fHistoryMenu->AddSeparatorItem();
	fHistoryMenu->AddItem(new BMenuItem(B_TRANSLATE("Export history" B_UTF8_ELLIPSIS),
		new BMessage(EXPORT_HISTORY)));
	fHistoryMenu->AddSeparatorItem();
	fHistoryMenuFixedItemCount = fHistoryMenu->CountItems();
	mainMenu->AddItem(fHistoryMenu);

	BPath bookmarkPath;
	entry_ref bookmarkRef;
	if (_BookmarkPath(bookmarkPath) == B_OK
		&& get_ref_for_path(bookmarkPath.Path(), &bookmarkRef) == B_OK) {
		BMenu* bookmarkMenu
			= new BookmarkMenu(B_TRANSLATE("Bookmarks"), this, &bookmarkRef);
		mainMenu->AddItem(bookmarkMenu);

		BDirectory barDir(&bookmarkRef);
		BEntry bookmarkBar(&barDir, kBookmarkBarSubdir);
		entry_ref bookmarkBarRef;

		bool bookmarkBarNotEmpty = false;
		if (bookmarkBar.Exists() && bookmarkBar.GetRef(&bookmarkBarRef)
				== B_OK) {
			BDirectory bookmarkBarDir(&bookmarkBarRef);
			if (bookmarkBarDir.InitCheck() == B_OK) {
				BEntry entry;
				if (bookmarkBarDir.GetNextEntry(&entry) == B_OK)
					bookmarkBarNotEmpty = true;
			}
		}

		if (bookmarkBarNotEmpty) {
			fBookmarkBar = new BookmarkBar("Bookmarks", this, &bookmarkBarRef);
			fBookmarkBarMenuItem->SetEnabled(true);
		} else
			fBookmarkBarMenuItem->SetEnabled(false);
	} else
		fBookmarkBarMenuItem->SetEnabled(false);

	// Back, Forward, Stop & Home buttons
	fBackButton = new BIconButton("Back", NULL, new BMessage(GO_BACK));
	fBackButton->SetIcon(201);
	fBackButton->TrimIcon();

	fForwardButton = new BIconButton("Forward", NULL, new BMessage(GO_FORWARD));
	fForwardButton->SetIcon(202);
	fForwardButton->TrimIcon();

	fStopButton = new BIconButton("Stop", NULL, new BMessage(STOP));
	fStopButton->SetIcon(204);
	fStopButton->TrimIcon();

	fHomeButton = new BIconButton("Home", NULL, new BMessage(HOME));
	fHomeButton->SetIcon(206);
	fHomeButton->TrimIcon();
	if (!fAppSettings->GetValue(kSettingsKeyShowHomeButton, true))
		fHomeButton->Hide();

	fDownloadsButton = new BIconButton("Downloads", NULL, new BMessage(SHOW_DOWNLOAD_WINDOW));
	fDownloadsButton->SetIcon(209); // Using existing icon resource ID, hopefully exists or placeholder
	fDownloadsButton->TrimIcon();
	if (!fAppSettings->GetValue(kSettingsKeyShowDownloadsButton, false))
		fDownloadsButton->Hide();

	fBookmarksButton = new BIconButton("Bookmarks", NULL, new BMessage(SHOW_BOOKMARKS));
	fBookmarksButton->SetIcon(208); // Placeholder ID
	fBookmarksButton->TrimIcon();
	if (!fAppSettings->GetValue(kSettingsKeyShowBookmarksButton, true))
		fBookmarksButton->Hide();

	// URL input group
	fURLInputGroup = new URLInputGroup(new BMessage(GOTO_URL));
	if (privateWindow)
		fURLInputGroup->SetPrivateMode(true);

	// Status Bar
	fStatusText = new BStringView("status", "");
	fStatusText->SetAlignment(B_ALIGN_LEFT);
	fStatusText->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fStatusText->SetExplicitMinSize(BSize(150, 12));
		// Prevent the window from growing to fit a long status message...
	BFont font(be_plain_font);
	font.SetSize(ceilf(font.Size() * 0.8));
	fStatusText->SetFont(&font, B_FONT_SIZE);

	// Loading progress bar
	fLoadingProgressBar = new BStatusBar("progress");
	fLoadingProgressBar->SetMaxValue(100);
	fLoadingProgressBar->Hide();
	font_height height;
	font.GetHeight(&height);
	fLoadingProgressBar->SetBarHeight(height.ascent + height.descent);

	const float kInsetSpacing = 3;
	const float kElementSpacing = 5;

	// Find group
	fFindCloseButton = new CloseButton(new BMessage(EDIT_HIDE_FIND_GROUP));
	fFindTextControl = new BTextControl("find", B_TRANSLATE("Find:"), "", NULL);
	fFindTextControl->SetModificationMessage(new BMessage(FIND_TEXT_CHANGED));
	fFindPreviousButton = new BButton(B_TRANSLATE("Previous"),
		new BMessage(EDIT_FIND_PREVIOUS));
	fFindPreviousButton->SetToolTip(
		B_TRANSLATE_COMMENT("Find previous occurrence of search terms",
			"find bar previous button tooltip"));
	fFindNextButton = new BButton(B_TRANSLATE("Next"),
		new BMessage(EDIT_FIND_NEXT));
	fFindNextButton->SetToolTip(
		B_TRANSLATE_COMMENT("Find next occurrence of search terms",
			"find bar next button tooltip"));
	fFindCaseSensitiveCheckBox = new BCheckBox(B_TRANSLATE("Match case"));
	BGroupLayout* findGroup = BLayoutBuilder::Group<>(B_VERTICAL, 0.0)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.Add(fFindCloseButton)
			.Add(fFindTextControl)
			.Add(fFindPreviousButton)
			.Add(fFindNextButton)
			.Add(fFindCaseSensitiveCheckBox)
			.SetInsets(kInsetSpacing, kInsetSpacing,
				kInsetSpacing, kInsetSpacing)
		)
	;

	// Navigation group
	BGroupLayout* navigationGroup = BLayoutBuilder::Group<>(B_VERTICAL, 0.0)
		.Add(BLayoutBuilder::Group<>(B_HORIZONTAL, kElementSpacing)
			.Add(fBackButton)
			.Add(fForwardButton)
			.Add(fStopButton)
			.Add(fHomeButton)
			.Add(fDownloadsButton)
			.Add(fBookmarksButton)
			.Add(fURLInputGroup)
			.SetInsets(kInsetSpacing, kInsetSpacing, kInsetSpacing,
				kInsetSpacing)
		)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
	;

	// Status bar group
	BGroupLayout* statusGroup = BLayoutBuilder::Group<>(B_VERTICAL, 0.0)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
		.Add(BLayoutBuilder::Group<>(B_HORIZONTAL, kElementSpacing)
			.Add(fStatusText)
			.Add(fLoadingProgressBar, 0.2)
			.AddStrut(12 - kElementSpacing)
			.SetInsets(kInsetSpacing, 0, kInsetSpacing, 0)
		)
	;

	BBitmapButton* toggleFullscreenButton = new BBitmapButton(kWindowIconBits,
		kWindowIconWidth, kWindowIconHeight, kWindowIconFormat,
		new BMessage(TOGGLE_FULLSCREEN));
	toggleFullscreenButton->SetBackgroundMode(BBitmapButton::MENUBAR_BACKGROUND);

#if !INTEGRATE_MENU_INTO_TAB_BAR
	BMenu* mainMenuItem = mainMenu;
	fMenuGroup = (new BGroupView(B_HORIZONTAL, 0))->GroupLayout();
#else
	BMenu* mainMenuItem = new BMenuBar("Main menu");
	mainMenuItem->AddItem(mainMenu);
	fMenuGroup = fTabManager->MenuContainerLayout();
#endif
	BLayoutBuilder::Group<>(fMenuGroup)
		.Add(mainMenuItem)
		.Add(toggleFullscreenButton, 0.0f)
	;

	if (fAppSettings->GetValue(kSettingsShowBookmarkBar, true))
		_ShowBookmarkBar(true);
	else
		_ShowBookmarkBar(false);

	fSavePanel.reset(new BFilePanel(B_SAVE_PANEL, new BMessenger(this), NULL, 0,
		false));

	// Layout
	BGroupView* topView = new BGroupView(B_VERTICAL, 0.0);

#if !INTEGRATE_MENU_INTO_TAB_BAR
	topView->AddChild(fMenuGroup);
#endif
	topView->AddChild(fTabManager->TabGroup());
	topView->AddChild(navigationGroup);
	if (fBookmarkBar != NULL)
		topView->AddChild(fBookmarkBar);
	topView->AddChild(fTabManager->ContainerView());
	topView->AddChild(findGroup);
	topView->AddChild(statusGroup);

	AddChild(topView);

	fURLInputGroup->MakeFocus(true);

	fTabGroup = fTabManager->TabGroup()->GetLayout();
	fNavigationGroup = navigationGroup;
	fFindGroup = findGroup;
	fStatusGroup = statusGroup;
	fToggleFullscreenButton = layoutItemFor(toggleFullscreenButton);

	fFindGroup->SetVisible(false);
	fToggleFullscreenButton->SetVisible(false);

	// Apply toolbar placement logic
	if (fToolbarBottom)
		_UpdateToolbarPlacement();

	CreateNewTab(url, true, webView);
	_ShowInterface(true);
	_SetAutoHideInterfaceInFullscreen(fAppSettings->GetValue(
		kSettingsKeyAutoHideInterfaceInFullscreenMode,
		fAutoHideInterfaceInFullscreenMode));

	AddShortcut('F', B_COMMAND_KEY | B_SHIFT_KEY,
		new BMessage(EDIT_HIDE_FIND_GROUP));
	AddShortcut('H', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(HOME));

	// Add shortcuts to select a particular tab
	for (int32 i = 1; i <= 9; i++) {
		BMessage* selectTab = new BMessage(SELECT_TAB);
		selectTab->AddInt32("tab index", i - 1);
		char numStr[2];
		snprintf(numStr, sizeof(numStr), "%d", (int) i);
		AddShortcut(numStr[0], B_COMMAND_KEY, selectTab);
	}

	// Add shortcut to cycle through tabs like in every other web browser
	AddShortcut(B_TAB, B_COMMAND_KEY, new BMessage(CYCLE_TABS));

	BKeymap keymap;
	keymap.SetToCurrent();
	BStringList unmodified(3);
	if (keymap.GetModifiedCharacters("+", B_SHIFT_KEY, 0, unmodified)
			== B_OK) {
		int32 count = unmodified.CountStrings();
		for (int32 i = 0; i < count; i++) {
			uint32 key = BUnicodeChar::FromUTF8(unmodified.StringAt(i));
			if (!HasShortcut(key, B_COMMAND_KEY)) {
				// Add semantic zoom in shortcut, bug #7428
				AddShortcut(key, B_COMMAND_KEY,
					new BMessage(ZOOM_FACTOR_INCREASE));
			}
		}
	}
	unmodified.MakeEmpty();

	// Add zoom out shortcut
	if (keymap.GetModifiedCharacters("-", 0, 0, unmodified) == B_OK) {
		int32 count = unmodified.CountStrings();
		for (int32 i = 0; i < count; i++) {
			uint32 key = BUnicodeChar::FromUTF8(unmodified.StringAt(i));
			if (!HasShortcut(key, B_COMMAND_KEY)) {
				AddShortcut(key, B_COMMAND_KEY,
					new BMessage(ZOOM_FACTOR_DECREASE));
			}
		}
	}
	unmodified.MakeEmpty();

	// Add zoom reset shortcut
	if (keymap.GetModifiedCharacters("0", 0, 0, unmodified) == B_OK) {
		int32 count = unmodified.CountStrings();
		for (int32 i = 0; i < count; i++) {
			uint32 key = BUnicodeChar::FromUTF8(unmodified.StringAt(i));
			if (!HasShortcut(key, B_COMMAND_KEY)) {
				AddShortcut(key, B_COMMAND_KEY,
					new BMessage(ZOOM_FACTOR_RESET));
			}
		}
	}
	unmodified.MakeEmpty();

	BMessage memMsg(CHECK_MEMORY_PRESSURE);
	fMemoryPressureRunner.reset(new BMessageRunner(BMessenger(this), &memMsg, 30000000)); // 30 seconds

	be_app->PostMessage(WINDOW_OPENED);
}


BrowserWindow::~BrowserWindow()
{
	fAppSettings->RemoveListener(BMessenger(this));
	fPulseRunner.reset();
	fButtonResetRunner.reset();
	fMemoryPressureRunner.reset();
	fSavePanel.reset();
	fFormSafetyHelper.reset();
	if (fPermissionsWindow) {
		if (fPermissionsWindow->Lock()) {
			fPermissionsWindow->PrepareToQuit();
			fPermissionsWindow->Quit();
		}
	}
	if (fNetworkWindow) {
		if (fNetworkWindow->Lock()) {
			fNetworkWindow->PrepareToQuit();
			fNetworkWindow->Quit();
		}
	}

	if (fTabSearchWindow) {
		if (fTabSearchWindow->Lock()) {
			fTabSearchWindow->Quit();
			fTabSearchWindow = NULL;
		}
	}
	fTabManager.reset();
}


void
BrowserWindow::DispatchMessage(BMessage* message, BHandler* target)
{
	const char* bytes;
	int32 modifierKeys;
	if ((message->what == B_KEY_DOWN || message->what == B_UNMAPPED_KEY_DOWN)
		&& message->FindString("bytes", &bytes) == B_OK
		&& message->FindInt32("modifiers", &modifierKeys) == B_OK) {
		if (bytes[0] == B_FUNCTION_KEY) {
			// Some function key Firefox compatibility
			int32 key;
			if (message->FindInt32("key", &key) == B_OK) {
				switch (key) {
					case B_F5_KEY:
						PostMessage(RELOAD);
						break;

					case B_F11_KEY:
						PostMessage(TOGGLE_FULLSCREEN);
						break;

					default:
						break;
				}
			}
		} else if (target == fURLInputGroup->TextView()) {
			// Handle B_RETURN in the URL text control. This is the easiest
			// way to react *only* when the user presses the return key in the
			// address bar, as opposed to trying to load whatever is in there
			// when the text control just goes out of focus.
			if (bytes[0] == B_RETURN) {
				// Do it in such a way that the user sees the Go-button go down.
				_InvokeButtonVisibly(fURLInputGroup->GoButton());
				return;
			} else if (bytes[0] == B_ESCAPE) {
				// Replace edited text with the current URL.
				fURLInputGroup->LockURLInput(false);
				fURLInputGroup->SetText(CurrentWebView()->MainFrameURL());
			}
		} else if (target == fFindTextControl->TextView()) {
			// Handle B_RETURN when the find text control has focus.
			if (bytes[0] == B_RETURN) {
				if ((modifierKeys & B_SHIFT_KEY) != 0)
					_InvokeButtonVisibly(fFindPreviousButton);
				else
					_InvokeButtonVisibly(fFindNextButton);
				return;
			} else if (bytes[0] == B_ESCAPE) {
				_InvokeButtonVisibly(fFindCloseButton);
				return;
			}
		} else if (bytes[0] == B_ESCAPE && !fMenusRunning) {
			if (modifierKeys == B_COMMAND_KEY)
				_ShowInterface(true);
			else {
				// Default escape key behavior:
				PostMessage(STOP);
				return;
			}
		}
	}

	if (message->what == B_MOUSE_MOVED || message->what == B_MOUSE_DOWN
		|| message->what == B_MOUSE_UP) {
		message->FindPoint("where", &fLastMousePos);
		if (message->FindInt64("when", &fLastMouseMovedTime) != B_OK)
			fLastMouseMovedTime = system_time();
		_CheckAutoHideInterface();
	}

	if (message->what == B_MOUSE_WHEEL_CHANGED) {
		BPoint where;
		uint32 buttons;
		CurrentWebView()->GetMouse(&where, &buttons, false);
		// Only do this when the mouse is over the web view
		if (CurrentWebView()->Bounds().Contains(where)) {
			// Zoom and unzoom text on Command + mouse wheel.
			// This could of course (and maybe should be) implemented in the
			// WebView, but there would need to be a way for the WebView to
			// know the setting of the fZoomTextOnly member here. Plus other
			// clients of the API may not want this feature.
			if ((modifiers() & B_COMMAND_KEY) != 0) {
				float deltaY;
				if (message->FindFloat("be:wheel_delta_y", &deltaY) == B_OK) {
					if (deltaY < 0)
						CurrentWebView()->IncreaseZoomFactor(fZoomTextOnly);
					else
						CurrentWebView()->DecreaseZoomFactor(fZoomTextOnly);

					return;
				}
			}
		} else {
			// Also don't scroll up and down if the mouse is not over the
			// web view
			return;
		}
	}

	BWebWindow::DispatchMessage(message, target);
}


void
BrowserWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case OPEN_LOCATION:
			_ShowInterface(true);
			if (fURLInputGroup->TextView()->IsFocus())
				fURLInputGroup->TextView()->SelectAll();
			else
				fURLInputGroup->MakeFocus(true);
			break;

		case RELOAD:
		{
			BWebView* webView = NULL;
			int32 tabIndex = -1;
			if (message->FindInt32("tab index", &tabIndex) == B_OK)
				webView = dynamic_cast<BWebView*>(fTabManager->ViewForTab(tabIndex));
			else
				webView = CurrentWebView();

			if (webView)
				webView->Reload();
			break;
		}
		case RELOAD_BYPASS_CACHE:
		{
			BWebView* webView = NULL;
			int32 tabIndex = -1;
			if (message->FindInt32("tab index", &tabIndex) == B_OK)
				webView = dynamic_cast<BWebView*>(fTabManager->ViewForTab(tabIndex));
			else
				webView = CurrentWebView();

			if (webView) {
				// To simulate "Bypass Cache", we set the cache model to DOCUMENT_VIEWER
				// (assuming this reduces caching) and set a flag to revert it later.
				// We cannot revert immediately because Reload() is async.
				if (webView->WebPage()) {
					// Use a constant that implies less caching if possible.
					// Assuming DOCUMENT_BROWSER (default?) vs something else.
					// If standard is WEB_BROWSER, maybe DOCUMENT_VIEWER is stricter?
					// I'll stick to what I used but persistent:
					webView->WebPage()->SetCacheModel(B_WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER);
					fIsBypassingCache = true;
				}
				webView->Reload();
			}
			break;
		}

		case SHOW_HIDE_BOOKMARK_BAR:
			_ShowBookmarkBar(fBookmarkBar->IsHidden());
			if (fAutoHideBookmarkBarMenuItem)
				fAutoHideBookmarkBarMenuItem->SetEnabled(!fBookmarkBar->IsHidden());
			break;

		case TOGGLE_AUTO_HIDE_BOOKMARK_BAR:
			fAutoHideBookmarkBar = !fAutoHideBookmarkBar;
			fAutoHideBookmarkBarMenuItem->SetMarked(fAutoHideBookmarkBar);
			_CheckAutoHideInterface();
			break;

		case GOTO_URL:
		{
			BString url;
			if (message->FindString("url", &url) != B_OK)
				url = fURLInputGroup->Text();

			_SetPageIcon(CurrentWebView(), NULL);
			_SmartURLHandler(url);

			break;
		}

		case SAVE_PAGE:
		{
			BMessage saveMsg(B_SAVE_REQUESTED);
			saveMsg.AddInt32("type", SAVE_PAGE);
			fSavePanel->SetMessage(&saveMsg);
			fSavePanel->SetSaveText(CurrentWebView()->MainFrameTitle());
			fSavePanel->Show();
			break;
		}

		case EXPORT_BOOKMARKS:
		{
			BMessage saveMsg(B_SAVE_REQUESTED);
			saveMsg.AddInt32("type", EXPORT_BOOKMARKS);
			fSavePanel->SetMessage(&saveMsg);
			fSavePanel->SetSaveText("bookmarks.html");
			fSavePanel->Show();
			break;
		}

		case IMPORT_BOOKMARKS:
		{
			BFilePanel* panel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this),
				NULL, B_FILE_NODE, false, new BMessage(IMPORT_BOOKMARKS));
			panel->Show();
			break;
		}

		case EXPORT_COOKIES:
		{
			BMessage saveMsg(B_SAVE_REQUESTED);
			saveMsg.AddInt32("type", EXPORT_COOKIES);
			fSavePanel->SetMessage(&saveMsg);
			fSavePanel->SetSaveText("cookies.txt");
			fSavePanel->Show();
			break;
		}

		case EXPORT_HISTORY:
		{
			BMessage saveMsg(B_SAVE_REQUESTED);
			saveMsg.AddInt32("type", EXPORT_HISTORY);
			fSavePanel->SetMessage(&saveMsg);
			fSavePanel->SetSaveText("history.csv");
			fSavePanel->Show();
			break;
		}

		case SYNC_EXPORT:
		{
			BFilePanel* panel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this),
				NULL, B_DIRECTORY_NODE, false, new BMessage(SYNC_EXPORT));
			panel->SetSaveText("WebPositiveProfile");
			panel->Show();
			break;
		}

		case SYNC_IMPORT:
		{
			BFilePanel* panel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this),
				NULL, B_DIRECTORY_NODE, false, new BMessage(SYNC_IMPORT));
			panel->Show();
			break;
		}

		case B_SAVE_REQUESTED:
		{
			entry_ref ref;
			BString name;
			int32 type = SAVE_PAGE;
			message->FindInt32("type", &type);

			if (message->FindRef("directory", &ref) == B_OK
				&& message->FindString("name", &name) == B_OK) {
				BDirectory dir(&ref);

				if (type == EXPORT_BOOKMARKS) {
					BPath path(&ref);
					path.Append(name);
					status_t status = BookmarkManager::ExportBookmarks(path);
					if (status != B_OK) {
						BString errorMsg(B_TRANSLATE("Failed to export bookmarks"));
						errorMsg << ": " << strerror(status);
						BAlert* alert = new BAlert(B_TRANSLATE("Export error"),
							errorMsg.String(), B_TRANSLATE("OK"));
						alert->Go();
					}
				} else if (type == EXPORT_HISTORY) {
					BPath path(&ref);
					path.Append(name);
					status_t status = BrowsingHistory::ExportHistory(path);
					if (status != B_OK) {
						BString errorMsg(B_TRANSLATE("Failed to export history"));
						errorMsg << ": " << strerror(status);
						BAlert* alert = new BAlert(B_TRANSLATE("Export error"),
							errorMsg.String(), B_TRANSLATE("OK"));
						alert->Go();
					}
				} else if (type == EXPORT_COOKIES) {
					BPath path(&ref);
					path.Append(name);
					status_t status = Sync::ExportCookies(path, fContext->GetCookieJar());
					if (status != B_OK) {
						BString errorMsg(B_TRANSLATE("Failed to export cookies"));
						errorMsg << ": " << strerror(status);
						BAlert* alert = new BAlert(B_TRANSLATE("Export error"),
							errorMsg.String(), B_TRANSLATE("OK"));
						alert->Go();
					}
				} else if (type == SAVE_PAGE) {
					BFile output(&dir, name,
						B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
					CurrentWebView()->WebPage()->GetContentsAsMHTML(output);
				}
			}
			break;
		}

		case SYNC_EXPORT:
		{
			// BFilePanel for directory selection might send this if we set the message
			entry_ref ref;
			BString name;
			if (message->FindRef("directory", &ref) == B_OK
				&& message->FindString("name", &name) == B_OK) {
				BPath path(&ref);
				path.Append(name);

				SyncParams* params = new SyncParams;
				params->path = path;
				params->context = fContext.Get();
				params->context->Acquire();
				params->target = BMessenger(this);

				thread_id thread = spawn_thread(_ExportProfileThread, "Export Profile",
					B_NORMAL_PRIORITY, params);
				if (thread >= 0) {
					if (resume_thread(thread) != B_OK) {
						kill_thread(thread);
						params->context->Release();
						delete params;
					}
				} else {
					params->context->Release();
					delete params;
				}
			}
			break;
		}

		case SYNC_IMPORT:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK) {
				BPath path(&ref);

				SyncParams* params = new SyncParams;
				params->path = path;
				params->context = fContext.Get();
				params->context->Acquire();
				params->target = BMessenger(this);

				thread_id thread = spawn_thread(_ImportProfileThread, "Import Profile",
					B_NORMAL_PRIORITY, params);
				if (thread >= 0) {
					if (resume_thread(thread) != B_OK) {
						kill_thread(thread);
						params->context->Release();
						delete params;
					}
				} else {
					params->context->Release();
					delete params;
				}
			}
			break;
		}

		case GO_BACK:
			CurrentWebView()->GoBack();
			break;

		case GO_FORWARD:
			CurrentWebView()->GoForward();
			break;

		case STOP:
			CurrentWebView()->StopLoading();
			break;

		case HOME:
			CurrentWebView()->LoadURL(fStartPageURL);
			break;

		case CLEAR_HISTORY: {
			BrowsingHistory* history = BrowsingHistory::DefaultInstance();
			if (history->CountItems() == 0)
				break;
			BAlert* alert = new BAlert(B_TRANSLATE("Confirmation"),
				B_TRANSLATE("Do you really want to "
				"clear the browsing history?"), B_TRANSLATE("Clear"),
				B_TRANSLATE("Cancel"));
			alert->SetShortcut(1, B_ESCAPE);

			if (alert->Go() == 0)
				history->Clear();
			break;
		}

		case CREATE_BOOKMARK:
		{
			BString fileName;
			BString title(CurrentWebView()->MainFrameTitle());
			BString url(CurrentWebView()->MainFrameURL());

			BBitmap* miniIcon = NULL;
			const BBitmap* largeIcon = NULL;
			PageUserData* userData = static_cast<PageUserData*>(CurrentWebView()->GetUserData());
			if (userData != NULL && userData->PageIcon() != NULL) {
				miniIcon = new BBitmap(BRect(0, 0, 15, 15), B_BITMAP_NO_SERVER_LINK, B_CMAP8);
				miniIcon->ImportBits(userData->PageIcon()->Bits(), userData->PageIcon()->BitsLength(), userData->PageIcon()->BytesPerRow(), 0, userData->PageIcon()->ColorSpace());
				largeIcon = userData->PageIconLarge();
			}

			BookmarkManager::CreateBookmark(fileName, title, url, miniIcon, largeIcon);
			delete miniIcon;
			break;
		}

		case SHOW_BOOKMARKS:
			BookmarkManager::ShowBookmarks();
			break;

		case IMPORT_BOOKMARKS:
		{
			// Handle Import from FilePanel selection
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK) {
				BPath path(&ref);
				status_t status = BookmarkManager::ImportBookmarks(path);
				if (status != B_OK) {
					BString errorMsg(B_TRANSLATE("Failed to import bookmarks"));
					errorMsg << ": " << strerror(status);
					BAlert* alert = new BAlert(B_TRANSLATE("Import error"),
						errorMsg.String(), B_TRANSLATE("OK"));
					alert->Go();
				}
			}
			break;
		}

		case B_REFS_RECEIVED:
		{
			// Currently the only source of these messages is the bookmarks
			// menu.
			// Filter refs into URLs, this also gets rid of refs for folders.
			// For clicks on sub-folders in the bookmarks menu, we have Tracker
			// open the corresponding folder.
			entry_ref ref;
			uint32 addedCount = 0;
			for (int32 i = 0; message->FindRef("refs", i, &ref) == B_OK; i++) {
				BEntry entry(&ref);
				uint32 addedSubCount = 0;
				if (entry.IsDirectory()) {
					BDirectory directory(&entry);
					BookmarkManager::AddBookmarkURLsRecursively(directory, message,
						addedSubCount);
				} else {
					BFile file(&ref, B_READ_ONLY);
					BString url;
					if (BookmarkManager::ReadURLAttr(file, url)) {
						message->AddString("url", url.String());
						addedSubCount++;
					}
				}
				if (addedSubCount == 0) {
					// Don't know what to do with this entry, just pass it
					// on to the system to handle. Note that this may result
					// in us opening other supported files via the application
					// mechanism.
					be_roster->Launch(&ref);
				}
				addedCount += addedSubCount;
			}
			message->RemoveName("refs");
			if (addedCount > 10) {
				BString string(B_TRANSLATE_COMMENT("Do you want to open "
					"%addedCount bookmarks all at once?", "Don't translate "
					"variable %addedCount."));
				string.ReplaceFirst("%addedCount", BString() << addedCount);

				BAlert* alert = new BAlert(
					B_TRANSLATE("Open bookmarks confirmation"),
					string.String(), B_TRANSLATE("Cancel"),
					B_TRANSLATE("Open all"));
				alert->SetShortcut(0, B_ESCAPE);
				if (alert->Go() == 0)
					break;
			}
			message->AddPointer("window", this);
			be_app->PostMessage(message);
			break;
		}

		case B_SIMPLE_DATA:
		{
			const char* filetype = message->GetString("be:filetypes");
			if (filetype != NULL
				&& strcmp(filetype, "application/x-vnd.Be-bookmark") == 0
				&& LastMouseMovedView() == fBookmarkBar) {
				// Something that can be made into a bookmark (e.g. the page icon)
				// was dragged and dropped on the bookmark bar.
				BPath path;
				if (BookmarkManager::GetBookmarkPath(path) == B_OK && path.Append(kBookmarkBarSubdir) == B_OK) {
					entry_ref ref;
					if (BEntry(path.Path()).GetRef(&ref) == B_OK) {
						message->AddRef("directory", &ref);
							// Add under the same name that Tracker would use, if
							// the ref had been added by dragging and dropping to Tracker.
						BookmarkManager::CreateBookmarkFromMessage(message);
					}
				}
				break;
			}

			// User possibly dropped files on this window.
			// If there is more than one entry_ref, let the app handle it
			// (open one new page per ref). If there is one ref, open it in
			// this window.
			type_code type;
			int32 countFound;
			if (message->GetInfo("refs", &type, &countFound) != B_OK
				|| type != B_REF_TYPE) {
				break;
			}
			if (countFound > 1) {
				message->what = B_REFS_RECEIVED;
				be_app->PostMessage(message);
				break;
			}
			entry_ref ref;
			if (message->FindRef("refs", &ref) != B_OK)
				break;
			BEntry entry(&ref, true);
			BPath path;
			if (!entry.Exists() || entry.GetPath(&path) != B_OK)
				break;

			BUrl url(path);
			CurrentWebView()->LoadURL(url);
			break;
		}

		case ZOOM_FACTOR_INCREASE:
			CurrentWebView()->IncreaseZoomFactor(fZoomTextOnly);
			{
				float z = CurrentWebView()->ZoomFactor(false);
				BString domain = BUrl(CurrentWebView()->MainFrameURL()).Host();
				SitePermissionsManager::Instance()->SetZoom(domain.String(), z);
			}
			break;
		case ZOOM_FACTOR_DECREASE:
			CurrentWebView()->DecreaseZoomFactor(fZoomTextOnly);
			{
				float z = CurrentWebView()->ZoomFactor(false);
				BString domain = BUrl(CurrentWebView()->MainFrameURL()).Host();
				SitePermissionsManager::Instance()->SetZoom(domain.String(), z);
			}
			break;
		case ZOOM_FACTOR_RESET:
			CurrentWebView()->ResetZoomFactor();
			{
				BString domain = BUrl(CurrentWebView()->MainFrameURL()).Host();
				SitePermissionsManager::Instance()->SetZoom(domain.String(), 1.0);
			}
			break;
		case ZOOM_TEXT_ONLY:
		{
			fZoomTextOnly = !fZoomTextOnly;
			fZoomTextOnlyMenuItem->SetMarked(fZoomTextOnly);
			BWebView* webView = CurrentWebView();
			if (webView != NULL) {
				float zoomFactor = webView->ZoomFactor(!fZoomTextOnly);
				webView->SetZoomFactor(1.0, !fZoomTextOnly);
				webView->SetZoomFactor(zoomFactor, fZoomTextOnly);
			}
			break;
		}

		case SHOW_NETWORK_WINDOW:
		{
			if (fNetworkWindow) {
				if (fNetworkWindow->IsHidden())
					fNetworkWindow->Show();
				else
					fNetworkWindow->Activate();
			} else {
				fNetworkWindow = new NetworkWindow(BRect(150, 150, 600, 500));
				fNetworkWindow->SetTarget(BMessenger(this));
				fNetworkWindow->Show();
			}
			break;
		}

		case NETWORK_WINDOW_CLOSED:
			fNetworkWindow = NULL;
			break;

		case TOGGLE_FULLSCREEN:
			ToggleFullscreen();
			break;

		case TOGGLE_AUTO_HIDE_INTERFACE_IN_FULLSCREEN:
			_SetAutoHideInterfaceInFullscreen(
				!fAutoHideInterfaceInFullscreenMode);
			break;

		case CHECK_AUTO_HIDE_INTERFACE:
			_CheckAutoHideInterface();
			break;

		case SEARCH_TABS:
		{
			if (fTabSearchWindow) {
				fTabSearchWindow->Activate();
			} else {
				fTabSearchWindow = new TabSearchWindow(fTabManager);
				fTabSearchWindow->Show();
			}
			break;
		}

		case TAB_SEARCH_WINDOW_QUIT:
			fTabSearchWindow = NULL;
			break;

		case SET_TAB_COLOR:
		{
			rgb_color color;
			if (message->FindColor("color", &color) == B_OK) {
				BWebView* webView = NULL;
				int32 tabIndex = -1;
				if (message->FindInt32("tab index", &tabIndex) == B_OK)
					webView = dynamic_cast<BWebView*>(fTabManager->ViewForTab(tabIndex));
				else
					webView = CurrentWebView();

				if (webView) {
					tabIndex = fTabManager->TabForView(webView);
					if (tabIndex >= 0) {
						TabView* tab = fTabManager->GetTabContainerView()->TabAt(tabIndex);
						if (tab) {
							tab->SetGroupColor(color);
						}
					}
				}
			}
			break;
		}

		case TOGGLE_READER_MODE:
			fReaderMode = !fReaderMode;
			fReaderModeMenuItem->SetMarked(fReaderMode);
			// Apply to current page
			if (CurrentWebView() && CurrentWebView()->WebPage()) {
				// Simple Reader Mode JS: Hide common non-article tags
				BString script = "if (typeof toggleReaderMode === 'undefined') {"
					"  toggleReaderMode = function(enable) {"
					"    var tags = ['nav', 'footer', 'aside', 'header', '.ads', '.sidebar'];"
					"    tags.forEach(function(tag) {"
					"      var elements = document.querySelectorAll(tag);"
					"      elements.forEach(function(el) {"
					"        el.style.display = enable ? 'none' : '';"
					"      });"
					"    });"
					"    document.body.style.maxWidth = enable ? '800px' : '';"
					"    document.body.style.margin = enable ? '0 auto' : '';"
					"    document.body.style.fontSize = enable ? '18px' : '';"
					"    document.body.style.lineHeight = enable ? '1.6' : '';"
					"  };"
					"}"
					"toggleReaderMode(";
				script << (fReaderMode ? "true" : "false") << ");";
				CurrentWebView()->WebPage()->ExecuteJavaScript(script);
			}
			break;

		case TOGGLE_TOOLBAR_BOTTOM:
			fToolbarBottom = !fToolbarBottom;
			fToolbarBottomMenuItem->SetMarked(fToolbarBottom);
			fAppSettings->SetValue(kSettingsKeyToolbarBottom, fToolbarBottom);
			_UpdateToolbarPlacement();
			break;

		case TOGGLE_DARK_MODE:
			fDarkMode = !fDarkMode;
			fDarkModeMenuItem->SetMarked(fDarkMode);
			// Apply to current page
			if (CurrentWebView() && CurrentWebView()->WebPage()) {
				BString script = "if (typeof toggleDarkMode === 'undefined') {"
					"  toggleDarkMode = function(enable) {"
					"    if (enable) {"
					"      var style = document.createElement('style');"
					"      style.id = 'webpositive-dark-mode';"
					"      style.innerHTML = 'html { filter: invert(100%); } img, video { filter: invert(100%); }';"
					"      document.head.appendChild(style);"
					"    } else {"
					"      var style = document.getElementById('webpositive-dark-mode');"
					"      if (style) style.remove();"
					"    }"
					"  };"
					"}"
					"toggleDarkMode(";
				script << (fDarkMode ? "true" : "false") << ");";
				CurrentWebView()->WebPage()->ExecuteJavaScript(script);
			}
			break;

		case SHOW_PAGE_SOURCE:
			CurrentWebView()->WebPage()->SendPageSource();
			break;
		case B_PAGE_SOURCE_RESULT:
			PageSourceSaver::HandlePageSourceResult(message);
			break;

		case INSPECT_ELEMENT:
			if (CurrentWebView() && CurrentWebView()->WebPage()) {
				fExpectingDomInspection = true;
				// Chunked transport to avoid console log limits/flooding
				BString script =
					"var html = document.documentElement.outerHTML;"
					"var chunkSize = 2048;"
					"var total = Math.ceil(html.length / chunkSize);"
					"console.log('INSPECT_DOM_START:' + total);"
					"for (var i = 0; i < total; i++) {"
					"  var chunk = html.substr(i * chunkSize, chunkSize);"
					"  console.log('INSPECT_DOM_CHUNK:' + i + ':' + chunk);"
					"}"
					"console.log('INSPECT_DOM_END');";
				CurrentWebView()->WebPage()->ExecuteJavaScript(script);
			}
			break;

		case TOGGLE_LOAD_IMAGES:
		{
			bool load = fAppSettings->GetValue(kSettingsKeyLoadImages, true);
			fAppSettings->SetValue(kSettingsKeyLoadImages, !load);
			// Apply immediately
			if (CurrentWebView() && CurrentWebView()->WebPage()) {
				BWebSettings* settings = CurrentWebView()->WebPage()->Settings();
				if (settings) settings->SetLoadsImagesAutomatically(!load);
			}
			fLoadImagesMenuItem->SetMarked(load);
			break;
		}

		case EDIT_FIND_NEXT:
			CurrentWebView()->FindString(fFindTextControl->Text(), true,
				fFindCaseSensitiveCheckBox->Value());
			break;
		case FIND_TEXT_CHANGED:
		{
			bool findTextAvailable = strlen(fFindTextControl->Text()) > 0;
			fFindPreviousMenuItem->SetEnabled(findTextAvailable);
			fFindNextMenuItem->SetEnabled(findTextAvailable);
			break;
		}
		case EDIT_FIND_PREVIOUS:
			CurrentWebView()->FindString(fFindTextControl->Text(), false,
				fFindCaseSensitiveCheckBox->Value());
			break;
		case EDIT_SHOW_FIND_GROUP:
			if (!fFindGroup->IsVisible())
				fFindGroup->SetVisible(true);
			fFindTextControl->MakeFocus(true);
			break;
		case EDIT_HIDE_FIND_GROUP:
			if (fFindGroup->IsVisible()) {
				fFindGroup->SetVisible(false);
				if (CurrentWebView() != NULL)
					CurrentWebView()->MakeFocus(true);
			}
			break;

		case B_CUT:
		case B_COPY:
		case B_PASTE:
		{
			BTextView* textView = dynamic_cast<BTextView*>(CurrentFocus());
			if (textView != NULL)
				textView->MessageReceived(message);
			else if (CurrentWebView() != NULL)
				CurrentWebView()->MessageReceived(message);
			break;
		}

		case B_EDITING_CAPABILITIES_RESULT:
		{
			BWebView* webView;
			if (message->FindPointer("view",
					reinterpret_cast<void**>(&webView)) != B_OK
				|| webView != CurrentWebView()) {
				break;
			}
			bool canCut;
			bool canCopy;
			bool canPaste;
			if (message->FindBool("can cut", &canCut) != B_OK)
				canCut = false;
			if (message->FindBool("can copy", &canCopy) != B_OK)
				canCopy = false;
			if (message->FindBool("can paste", &canPaste) != B_OK)
				canPaste = false;
			fCutMenuItem->SetEnabled(canCut);
			fCopyMenuItem->SetEnabled(canCopy);
			fPasteMenuItem->SetEnabled(canPaste);
			break;
		}

		case SHOW_DOWNLOAD_WINDOW:
		case SHOW_SETTINGS_WINDOW:
		case SHOW_CONSOLE_WINDOW:
		case SHOW_COOKIE_WINDOW:
			message->AddUInt32("workspaces", Workspaces());
			be_app->PostMessage(message);
			break;

		case SHOW_CERTIFICATE_WINDOW:
		{
			BString info;
			info << "Page Security Information:\n\n";

			BWebView* view = CurrentWebView();
			if (view) {
				BString url = view->MainFrameURL();
				info << "URL: " << url << "\n";
				if (url.StartsWith("https://")) {
					info << "Connection is encrypted (HTTPS).\n\n";
					info << "Note: Full certificate details are not yet available via the API.";
				} else {
					info << "Connection is NOT encrypted.";
				}
			} else {
				info << "No page loaded.";
			}
			BAlert* alert = new BAlert("Security Info", info.String(), "OK");
			alert->Go();
			break;
		}

		case SHOW_PERMISSIONS_WINDOW:
		{
			if (fPermissionsWindow) {
				if (fPermissionsWindow->IsHidden())
					fPermissionsWindow->Show();
				else
					fPermissionsWindow->Activate();
			} else {
				fPermissionsWindow = new PermissionsWindow(BRect(100, 100, 400, 400),
					fContext->GetCookieJar());
				fPermissionsWindow->SetTarget(BMessenger(this));
				fPermissionsWindow->Show();
			}
			break;
		}

		case PERMISSIONS_WINDOW_CLOSED:
			fPermissionsWindow = NULL;
			break;

		case CLOSE_TAB:
			if (fTabManager->CountTabs() > 1) {
				int32 index;
				if (message->FindInt32("tab index", &index) != B_OK)
					index = fTabManager->SelectedTabIndex();

				// If we are closing the selected tab, we need to decide which
				// tab to select next.
				int32 nextSelection = -1;
				if (index == fTabManager->SelectedTabIndex()) {
					if (index < fTabManager->CountTabs() - 1)
						nextSelection = index;
					else
						nextSelection = index - 1;
				}

				_ShutdownTab(index);
				_UpdateTabGroupVisibility();

				if (nextSelection >= 0)
					fTabManager->SelectTab(nextSelection);

			} else
				PostMessage(B_QUIT_REQUESTED);
			break;

		case CLOSE_OTHER_TABS:
		{
			int32 index;
			if (message->FindInt32("tab index", &index) == B_OK) {
				// Close tabs from end to start to avoid index shifting problems
				// Skip the target tab
				for (int32 i = fTabManager->CountTabs() - 1; i >= 0; i--) {
					if (i != index)
						_ShutdownTab(i);
				}
				_UpdateTabGroupVisibility();
			}
			break;
		}

		case CLOSE_TABS_TO_RIGHT:
		{
			int32 index;
			if (message->FindInt32("tab index", &index) == B_OK) {
				// Close tabs from end down to index + 1
				for (int32 i = fTabManager->CountTabs() - 1; i > index; i--) {
					_ShutdownTab(i);
				}
				_UpdateTabGroupVisibility();
			}
			break;
		}

		case SELECT_TAB:
		{
			int32 index;
			if (message->FindInt32("tab index", &index) == B_OK
				&& fTabManager->SelectedTabIndex() != index
				&& fTabManager->CountTabs() > index) {
				fTabManager->SelectTab(index);
			}

			break;
		}

		case CYCLE_TABS:
		{
			int32 index = fTabManager->SelectedTabIndex() + 1;
			if (index >= fTabManager->CountTabs())
				index = 0;
			fTabManager->SelectTab(index);
			break;
		}

		case REOPEN_CLOSED_TAB:
			_ReopenClosedTab();
			break;

		case REOPEN_CLOSED_TAB_WITH_INDEX:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK) {
				if (index >= 0 && index < (int32)fClosedTabs.size()) {
					ClosedTabInfo info = fClosedTabs[index];
					fClosedTabs.erase(fClosedTabs.begin() + index);
					_UpdateRecentlyClosedMenu();
					CreateNewTab(info.url, true);
				}
			}
			break;
		}

		case PIN_TAB:
		case UNPIN_TAB:
		{
			BWebView* webView = NULL;
			int32 tabIndex = -1;
			if (message->FindInt32("tab index", &tabIndex) == B_OK)
				webView = dynamic_cast<BWebView*>(fTabManager->ViewForTab(tabIndex));
			else
				webView = CurrentWebView();

			if (webView) {
				tabIndex = fTabManager->TabForView(webView);
				if (tabIndex >= 0) {
					TabView* tab = fTabManager->GetTabContainerView()->TabAt(tabIndex);
					if (tab) {
						bool pinned = (message->what == PIN_TAB);
						if (tab->IsPinned() == pinned) break;

						tab->SetPinned(pinned);

						int32 pinnedCount = 0;
						for (int32 i = 0; i < fTabManager->CountTabs(); i++) {
							TabView* t = fTabManager->GetTabContainerView()->TabAt(i);
							if (t && t->IsPinned() && t != tab) {
								pinnedCount++;
							}
						}

						if (tabIndex != pinnedCount)
							fTabManager->MoveTab(tabIndex, pinnedCount);
					}
				}
			}
			break;
		}

		case COPY_AS_MARKDOWN:
		case COPY_AS_HTML:
		case COPY_AS_PLAIN_TEXT:
		{
			BWebView* view = CurrentWebView();
			if (view == NULL)
				break;
			BString url = view->MainFrameURL();
			BString title = view->MainFrameTitle();
			if (title.Length() == 0)
				title = url;

			BString text;
			if (message->what == COPY_AS_MARKDOWN) {
				text << "[" << EscapeMarkdown(title) << "](" << EscapeMarkdownURL(url) << ")";
			} else if (message->what == COPY_AS_HTML) {
				text << "<a href=\"" << EscapeHTML(url) << "\">" << EscapeHTML(title) << "</a>";
			} else {
				text << title << ": " << url;
			}

			if (be_clipboard->Lock()) {
				be_clipboard->Clear();
				BMessage* clip = be_clipboard->Data();
				clip->AddData("text/plain", B_MIME_TYPE, text.String(),
					text.Length());
				be_clipboard->Commit();
				be_clipboard->Unlock();
			}
			break;
		}

		case TAB_CHANGED:
		{
			// This message may be received also when the last tab closed,
			// i.e. with index == -1.
			int32 index;
			if (message->FindInt32("tab index", &index) != B_OK)
				index = -1;
			_TabChanged(index);
			break;
		}

		case SETTINGS_VALUE_CHANGED:
		{
			BString name;
			if (message->FindString("name", &name) != B_OK)
				break;
			bool flag;
			BString string;
			uint32 value;
			if (name == kSettingsKeyShowTabsIfSinglePageOpen
				&& message->FindBool("value", &flag) == B_OK) {
				if (fShowTabsIfSinglePageOpen != flag) {
					fShowTabsIfSinglePageOpen = flag;
					_UpdateTabGroupVisibility();
				}
			} else if (name == kSettingsKeyAutoHidePointer
				&& message->FindBool("value", &flag) == B_OK) {
				fAutoHidePointer = flag;
				if (CurrentWebView())
					CurrentWebView()->SetAutoHidePointer(fAutoHidePointer);
			} else if (name == kSettingsKeyStartPageURL
				&& message->FindString("value", &string) == B_OK) {
				fStartPageURL = string;
			} else if (name == kSettingsKeySearchPageURL
				&& message->FindString("value", &string) == B_OK) {
				fSearchPageURL = string;
			} else if (name == kSettingsKeyNewWindowPolicy
				&& message->FindUInt32("value", &value) == B_OK) {
				fNewWindowPolicy = value;
			} else if (name == kSettingsKeyNewTabPolicy
				&& message->FindUInt32("value", &value) == B_OK) {
				fNewTabPolicy = value;
			} else if (name == kSettingsKeyAutoHideInterfaceInFullscreenMode
				&& message->FindBool("value", &flag) == B_OK) {
				_SetAutoHideInterfaceInFullscreen(flag);
			} else if (name == kSettingsKeyShowHomeButton
				&& message->FindBool("value", &flag) == B_OK) {
				if (flag)
					fHomeButton->Show();
				else
					fHomeButton->Hide();
			} else if (name == kSettingsKeyShowDownloadsButton
				&& message->FindBool("value", &flag) == B_OK) {
				if (flag)
					fDownloadsButton->Show();
				else
					fDownloadsButton->Hide();
			} else if (name == kSettingsKeyShowBookmarksButton
				&& message->FindBool("value", &flag) == B_OK) {
				if (flag)
					fBookmarksButton->Show();
				else
					fBookmarksButton->Hide();
			} else if (name == kSettingsShowBookmarkBar
				&& message->FindBool("value", &flag) == B_OK) {
				_ShowBookmarkBar(flag);
			} else if (name == kSettingsKeyDisableCache
				&& message->FindBool("value", &flag) == B_OK) {
				if (CurrentWebView() && CurrentWebView()->WebPage()) {
					if (flag)
						CurrentWebView()->WebPage()->SetCacheModel(B_WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER);
					else
						CurrentWebView()->WebPage()->SetCacheModel(B_WEBKIT_CACHE_MODEL_WEB_BROWSER);
				}
			} else if (name == kSettingsKeyLowRAMMode
				&& message->FindBool("value", &flag) == B_OK) {
				fLowRAMMode = flag;
				if (fLowRAMMode)
					BWebPage::SetCacheModel(B_WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER);
				else
					BWebPage::SetCacheModel(B_WEBKIT_CACHE_MODEL_WEB_BROWSER);
				_CheckMemoryPressure();
			} else if (name == kSettingsKeyLoadImages
				&& message->FindBool("value", &flag) == B_OK) {
				// flag is "load images" (true = load, false = don't load)
				// Menu item is "Disable images". So if flag is true, item should be unchecked.
				fLoadImagesMenuItem->SetMarked(!flag);
				if (CurrentWebView() && CurrentWebView()->WebPage()) {
					BWebSettings* settings = CurrentWebView()->WebPage()->Settings();
					if (settings) settings->SetLoadsImagesAutomatically(flag);
				}
			}
			break;
		}
		case ADD_CONSOLE_MESSAGE:
		{
			BString text;
			if (message->FindString("string", &text) == B_OK) {
				const char* kOpenPrivatePrefix = "OPEN_IN_PRIVATE_WINDOW:";
				if (text.StartsWith(kOpenPrivatePrefix)) {
					BString url = text;
					url.RemoveFirst(kOpenPrivatePrefix);
					BMessage* newPrivateWindowMessage = new BMessage(NEW_WINDOW);
					newPrivateWindowMessage->AddString("url", url);
					newPrivateWindowMessage->AddBool("private", true);
					be_app->PostMessage(newPrivateWindowMessage);
					break;
				}
				if (text.StartsWith("WebPositive:FormDirty:")) {
					fFormSafetyHelper->ConsoleMessage(text);
					break;
				}
				if (fExpectingDomInspection) {
					if (text.StartsWith("INSPECT_DOM_START:")) {
						fInspectDomBuffer = "";
						fInspectDomExpectedChunks = atoi(text.String() + strlen("INSPECT_DOM_START:"));
						fInspectDomReceivedChunks = 0;

						// Preallocate buffer to avoid frequent reallocations.
						// JS side uses 2048 chars per chunk.
						// Cap at 20MB to match the hard limit enforced below.
						int32 estimatedSize = fInspectDomExpectedChunks * 2048;
						if (estimatedSize > 20 * 1024 * 1024)
							estimatedSize = 20 * 1024 * 1024;

						if (estimatedSize > 0) {
							fInspectDomBuffer.LockBuffer(estimatedSize);
							fInspectDomBuffer.UnlockBuffer(0);
						}

						break; // Don't show in console
					} else if (text.StartsWith("INSPECT_DOM_CHUNK:")) {
						// Format: INSPECT_DOM_CHUNK:index:content
						// We assume ordered delivery for now (console usually is),
						// but rigorous impl would buffer by index.
						// Simple append for now as JS execution is single threaded usually.
						int32 firstColon = text.FindFirst(':', strlen("INSPECT_DOM_CHUNK:"));
						// Cap at 20MB to prevent DoS
						if (firstColon > 0 && fInspectDomBuffer.Length() < 20 * 1024 * 1024) {
							fInspectDomBuffer << (text.String() + firstColon + 1);
							fInspectDomReceivedChunks++;
						}
						break;
					} else if (text.StartsWith("INSPECT_DOM_END")) {
						fExpectingDomInspection = false;
						BMessage msg(B_PAGE_SOURCE_RESULT);
						msg.AddString("source", fInspectDomBuffer);
						msg.AddString("url", CurrentWebView()->MainFrameURL());
						PageSourceSaver::HandlePageSourceResult(&msg);
						fInspectDomBuffer = "";
						break;
					}
				}
			}
			be_app->PostMessage(message);
			BWebWindow::MessageReceived(message);
			break;
		}

		case CHECK_FORM_DIRTY_TIMEOUT:
			fFormSafetyHelper->MessageReceived(message);
			break;

		case CHECK_MEMORY_PRESSURE:
			_CheckMemoryPressure();
			break;

		case RESET_BUTTON_STATE:
		{
			fButtonResetRunner.reset();

			BButton* button = NULL;
			if (message->FindPointer("button", (void**)&button) == B_OK) {
				button->SetValue(B_CONTROL_OFF);
			}
			break;
		}

		case B_COPY_TARGET:
		{
			const char* filetype = message->GetString("be:filetypes");
			if (filetype != NULL && strcmp(filetype, "application/x-vnd.Be-bookmark") == 0) {
				// Tracker replied after the user dragged and dropped something
				// that can be bookmarked (e.g. the page icon) to a Tracker window.
				BookmarkManager::CreateBookmarkFromMessage(message);
				break;
			} else {
				BWebWindow::MessageReceived(message);
				break;
			}
		}

		default:
			BWebWindow::MessageReceived(message);
			break;
	}
}


status_t
BrowserWindow::Archive(BMessage* archive, bool deep) const
{
	status_t status = archive->AddRect("window frame", Frame());
	if (status == B_OK)
		status = archive->AddUInt32("window workspaces", Workspaces());

	for (int32 i = 0; i < fTabManager->CountTabs(); i++) {
		BWebView* view = dynamic_cast<BWebView*>(fTabManager->ViewForTab(i));
		if (view == NULL) {
			continue;
		}

		if (status == B_OK)
			status = archive->AddString("tab", view->MainFrameURL());
	}

	return status;
}


bool
BrowserWindow::QuitRequested()
{
	if (fFormSafetyHelper->QuitRequested()) {
		if (fTabSearchWindow) {
			fTabSearchWindow->Lock();
			fTabSearchWindow->Quit();
			fTabSearchWindow = NULL;
		}

		BMessage message(WINDOW_CLOSED);
		Archive(&message);

		// Iterate over all tabs to delete all BWebViews.
		// Do this here, so WebKit tear down happens earlier.
		SetCurrentWebView(NULL);
		while (fTabManager->CountTabs() > 0)
			_ShutdownTab(fTabManager->CountTabs() - 1);

		message.AddRect("window frame", WindowFrame());
		be_app->PostMessage(&message);
		return true;
	}
	return false;
}


void
BrowserWindow::MenusBeginning()
{
	_UpdateHistoryMenu();
	_UpdateClipboardItems();
	_UpdateRecentlyClosedMenu();

	if (fAutoHideBookmarkBarMenuItem && fBookmarkBar)
		fAutoHideBookmarkBarMenuItem->SetEnabled(!fBookmarkBar->IsHidden());

	// Tab pinning menu items
	// Check if the current view is a WebView (it should be)
	BWebView* currentWebView = CurrentWebView();
	if (currentWebView) {
		int32 tabIndex = fTabManager->TabForView(currentWebView);
		if (tabIndex >= 0) {
			TabView* tab = fTabManager->GetTabContainerView()->TabAt(tabIndex);
			if (tab) {
				// We don't have a "Tab" menu in the main menu bar, but we can add
				// these options to the "View" menu or context menu if accessible.
				// Since we can't easily access context menu here without modifying WebView,
				// let's add them to the "View" menu dynamically or just ensure they are handled.
				// Actually, the prompt implies "Add tab pinning".
				// Adding to the View menu is a safe bet for now.
				// Find the "View" menu.
				BMenu* viewMenu = NULL;
				BMenuBar* keyMenuBar = KeyMenuBar();
				if (keyMenuBar) {
					viewMenu = keyMenuBar->FindItem(B_TRANSLATE("View"))->Submenu();
				}

				if (viewMenu) {
					// Remove existing Pin/Unpin items if any to avoid duplicates
					// Simple check: Look for items with matching command
					BMenuItem* item = viewMenu->FindItem(PIN_TAB);
					if (item) {
						item->Menu()->RemoveItem(item);
						delete item;
					}
					item = viewMenu->FindItem(UNPIN_TAB);
					if (item) {
						item->Menu()->RemoveItem(item);
						delete item;
					}

					// Add appropriate item
					if (tab->IsPinned()) {
						viewMenu->AddItem(new BMenuItem(B_TRANSLATE("Unpin tab"),
							new BMessage(UNPIN_TAB)));
					} else {
						viewMenu->AddItem(new BMenuItem(B_TRANSLATE("Pin tab"),
							new BMessage(PIN_TAB)));
					}

					// Grouping (Color)
					// Only add if not already present
					if (viewMenu->FindItem(B_TRANSLATE("Set tab color")) == NULL) {
						BMenu* colorMenu = new BMenu(B_TRANSLATE("Set tab color"));
						const char* kColorNames[] = {"None", "Red", "Green", "Blue", "Yellow"};
						rgb_color kColors[] = {
							ui_color(B_PANEL_BACKGROUND_COLOR),
							{255, 100, 100, 255},
							{100, 255, 100, 255},
							{100, 100, 255, 255},
							{255, 255, 100, 255}
						};

						for (size_t i = 0; i < sizeof(kColorNames)/sizeof(char*); i++) {
							BMessage* colorMsg = new BMessage(SET_TAB_COLOR);
							colorMsg->AddColor("color", kColors[i]);
							colorMenu->AddItem(new BMenuItem(kColorNames[i], colorMsg));
						}
						viewMenu->AddItem(colorMenu);
					}
				}
			}
		}
	}

	fMenusRunning = true;
}


void
BrowserWindow::MenusEnded()
{
	fMenusRunning = false;
}


void
BrowserWindow::ScreenChanged(BRect screenSize, color_space format)
{
	if (fIsFullscreen)
		_ResizeToScreen();
}


void
BrowserWindow::WorkspacesChanged(uint32 oldWorkspaces, uint32 newWorkspaces)
{
	if (fIsFullscreen)
		_ResizeToScreen();
}


static bool
viewIsChild(const BView* parent, const BView* view)
{
	if (parent == view)
		return true;

	int32 count = parent->CountChildren();
	for (int32 i = 0; i < count; i++) {
		BView* child = parent->ChildAt(i);
		if (viewIsChild(child, view))
			return true;
	}
	return false;
}


void
BrowserWindow::SetCurrentWebView(BWebView* webView)
{
	if (webView == CurrentWebView())
		return;

	if (CurrentWebView() != NULL) {
		// Remember the currently focused view before switching tabs,
		// so that we can revert the focus when switching back to this tab
		// later.
		PageUserData* userData = static_cast<PageUserData*>(
			CurrentWebView()->GetUserData());
		if (userData == NULL) {
		userData = new PageUserData(CurrentFocus());
			CurrentWebView()->SetUserData(userData);
		}
		userData->SetFocusedView(CurrentFocus());
		userData->SetURLInputContents(fURLInputGroup->Text());
		int32 selectionStart;
		int32 selectionEnd;
		fURLInputGroup->TextView()->GetSelection(&selectionStart,
			&selectionEnd);
		userData->SetURLInputSelection(selectionStart, selectionEnd);

		// Capture Preview
		if (CurrentWebView() && !CurrentWebView()->IsHidden()) {
			BBitmap* tempPreview = new BBitmap(CurrentWebView()->Bounds(), B_RGB32);
			if (tempPreview->InitCheck() == B_OK) {
				// Use BScreen to read content if visible
				if (CurrentWebView()->Window()) {
					BScreen screen(CurrentWebView()->Window());
					BRect screenRect = CurrentWebView()->ConvertToScreen(CurrentWebView()->Bounds());
					if (screen.ReadBitmap(tempPreview, false, &screenRect) == B_OK) {
						// Scale down to save memory
						BRect thumbRect(0, 0, 200, 150);
						BBitmap* thumbnail = new BBitmap(thumbRect, B_RGB32, true);
						if (thumbnail->IsValid()) {
							BView* thumbView = new BView(thumbRect, "thumb", B_FOLLOW_NONE, B_WILL_DRAW);
							thumbnail->AddChild(thumbView);
							thumbnail->Lock();
							thumbView->SetHighColor(B_TRANSPARENT_32_BIT);
							thumbView->FillRect(thumbRect);
							thumbView->SetDrawingMode(B_OP_COPY);
							thumbView->DrawBitmap(tempPreview, tempPreview->Bounds(), thumbRect, B_FILTER_BITMAP_BILINEAR);
							thumbView->Sync();
							thumbnail->Unlock();
							thumbnail->RemoveChild(thumbView);
							delete thumbView;

							userData->SetPreview(thumbnail);
						}
						delete thumbnail;
					}
				}
			}
			delete tempPreview;
		}
	}

	BWebWindow::SetCurrentWebView(webView);

	if (webView != NULL) {
		webView->SetAutoHidePointer(fAutoHidePointer);

		_UpdateTitle(webView->MainFrameTitle());

		// Restore the previous focus or focus the web view.
		PageUserData* userData = static_cast<PageUserData*>(
			webView->GetUserData());
		BView* focusedView = NULL;
		if (userData != NULL) {
			focusedView = userData->FocusedView();
			fIsLoading = userData->IsLoading();
		} else {
			fIsLoading = false;
		}

		if (focusedView != NULL
			&& viewIsChild(GetLayout()->View(), focusedView)) {
			focusedView->MakeFocus(true);
		} else
			webView->MakeFocus(true);

		bool state = fURLInputGroup->IsURLInputLocked();
		fURLInputGroup->LockURLInput(false);
			// Unlock it so the following code can update the URL

		if (userData != NULL) {
			fURLInputGroup->SetPageIcon(userData->PageIcon());
			if (userData->URLInputContents().Length())
				fURLInputGroup->SetText(userData->URLInputContents());
			else
				fURLInputGroup->SetText(webView->MainFrameURL());
			if (userData->URLInputSelectionStart() >= 0) {
				fURLInputGroup->TextView()->Select(
					userData->URLInputSelectionStart(),
					userData->URLInputSelectionEnd());
			}
		} else {
			fURLInputGroup->SetPageIcon(NULL);
			fURLInputGroup->SetText(webView->MainFrameURL());
		}

		fURLInputGroup->LockURLInput(state);
			// Restore the state

		// Trigger update of the interface to the new page, by requesting
		// to resend all notifications.
		if (userData != NULL) {
			if (userData->IsLazy()) {
				userData->SetIsLazy(false);
				BString url = userData->PendingURL();
				if (url.Length() > 0)
					webView->LoadURL(url);
			} else if (userData->IsDiscarded()) {
				userData->SetIsDiscarded(false);
				BString url = userData->PendingURL();
				if (url.Length() > 0)
					webView->LoadURL(url);
			}
		}

		webView->WebPage()->ResendNotifications();
	} else
		_UpdateTitle("");
}


bool
BrowserWindow::IsBlankTab() const
{
	if (CurrentWebView() == NULL)
		return false;
	BString requestedURL = CurrentWebView()->MainFrameRequestedURL();
	return requestedURL.Length() == 0
		|| requestedURL == _NewTabURL(fTabManager->CountTabs() == 1);
}


void
BrowserWindow::CreateNewTab(const BString& _url, bool select,
	BWebView* webView, bool lazy)
{
	bool applyNewPagePolicy = webView == NULL;
	// Executed in app thread (new BWebPage needs to be created in app thread).
	if (webView == NULL)
		webView = new BWebView("web view", fContext);

	bool isNewWindow = fTabManager->CountTabs() == 0;

	fTabManager->AddTab(webView, B_TRANSLATE("New tab"));

	BString url(_url);
	if (applyNewPagePolicy && url.Length() == 0)
		url = _NewTabURL(isNewWindow);

	if (url.Length() > 0) {
		// Try loading favicon from cache for initial state
		_LoadFavicon(url, webView);

		if (lazy) {
			_SetPageIcon(webView, NULL); // Ensure userData exists
			PageUserData* userData = static_cast<PageUserData*>(webView->GetUserData());
			if (userData) {
				userData->SetIsLazy(true);
				userData->SetPendingURL(url);
			}
			// Don't load URL
		} else {
			webView->LoadURL(url.String());
		}
	}

	if (select) {
		fTabManager->SelectTab(fTabManager->CountTabs() - 1);
		SetCurrentWebView(webView);
		webView->WebPage()->ResendNotifications();
		fURLInputGroup->SetPageIcon(NULL);
		fURLInputGroup->SetText(url.String());
		fURLInputGroup->MakeFocus(true);
	}

	_ShowInterface(true);
	_UpdateTabGroupVisibility();
}


void
BrowserWindow::RestartDownload(const BString& url)
{
	CreateNewTab(url, false);
	BWebView* view = dynamic_cast<BWebView*>(
		fTabManager->ViewForTab(fTabManager->CountTabs() - 1));
	if (view != NULL) {
		PageUserData* data = static_cast<PageUserData*>(view->GetUserData());
		if (data == NULL) {
			data = new(std::nothrow) PageUserData(NULL);
			view->SetUserData(data);
		}
		if (data != NULL)
			data->SetIsDownloadRestart(true);
	}
}


BRect
BrowserWindow::WindowFrame() const
{
	if (fIsFullscreen)
		return fNonFullscreenWindowFrame;
	else
		return Frame();
}


void
BrowserWindow::ToggleFullscreen()
{
	if (fIsFullscreen) {
		MoveTo(fNonFullscreenWindowFrame.LeftTop());
		ResizeTo(fNonFullscreenWindowFrame.Width(),
			fNonFullscreenWindowFrame.Height());

		SetFlags(Flags() & ~(B_NOT_RESIZABLE | B_NOT_MOVABLE));
		SetLook(B_DOCUMENT_WINDOW_LOOK);

		_ShowInterface(true);
	} else {
		fNonFullscreenWindowFrame = Frame();
		_ResizeToScreen();

		SetFlags(Flags() | (B_NOT_RESIZABLE | B_NOT_MOVABLE));
		SetLook(B_TITLED_WINDOW_LOOK);
	}
	fIsFullscreen = !fIsFullscreen;
	fFullscreenItem->SetMarked(fIsFullscreen);
	fToggleFullscreenButton->SetVisible(fIsFullscreen);
}


// #pragma mark - Notification API


void
BrowserWindow::NewWindowRequested(const BString& url, bool primaryAction)
{
	// Always open new windows in the application thread, since
	// creating a BWebView will try to grab the application lock.
	// But our own WebPage may already try to lock us from within
	// the application thread -> dead-lock. Thus we can't wait for
	// a reply here.
	BMessage message(NEW_TAB);
	message.AddPointer("window", this);
	message.AddString("url", url);
	message.AddBool("select", primaryAction);
	be_app->PostMessage(&message);
}


void
BrowserWindow::NewPageCreated(BWebView* view, BRect windowFrame,
	bool modalDialog, bool resizable, bool activate)
{
	if (windowFrame.IsValid()) {
		BrowserWindow* window = new BrowserWindow(windowFrame, fAppSettings,
			BString(), fContext, INTERFACE_ELEMENT_STATUS,
			view);
		window->Show();
	} else
		CreateNewTab(BString(), activate, view);
}


void
BrowserWindow::CloseWindowRequested(BWebView* view)
{
	int32 index = fTabManager->TabForView(view);
	if (index < 0) {
		// Tab is already gone.
		return;
	}
	BMessage message(CLOSE_TAB);
	message.AddInt32("tab index", index);
	PostMessage(&message, this);
}


void
BrowserWindow::LoadNegotiating(const BString& url, BWebView* view)
{
	fExpectingDomInspection = false;
	if (fNetworkWindow) {
		BMessage msg(ADD_NETWORK_REQUEST);
		msg.AddString("url", url);
		fNetworkWindow->PostMessage(&msg);
	}

	// Ad-Block List
	if (fAppSettings->GetValue(kSettingsKeyBlockAds, false)) {
		static const std::vector<BString> kBlockedDomains = [] {
			std::vector<BString> domains;
			domains.push_back("adservice.google.com");
			domains.push_back("connect.facebook.net");
			domains.push_back("doubleclick.net");
			domains.push_back("facebook.net");
			domains.push_back("google-analytics.com");
			domains.push_back("googlesyndication.com");
			std::sort(domains.begin(), domains.end());
			return domains;
		}();

		BUrl checkUrl(url);
		if (checkUrl.IsValid()) {
			BString host = checkUrl.Host();
			host.ToLower();

			// Binary search for exact match
			if (std::binary_search(kBlockedDomains.begin(), kBlockedDomains.end(), host)) {
				if (view)
					view->LoadURL("about:blank");
				return;
			}

			// Check for subdomain match (e.g. ad.doubleclick.net)
			// This is still O(N) in worst case unless we optimize domain tree, but N is small (6).
			// With larger list, a trie or reversed domain sort would be better.
			// Given the constraint "sorted vector and binary search", exact match is fast.
			// For suffix match, we can lower_bound to find potential candidates.

			// Simple suffix check for now as per previous logic, but using vector
			for (const auto& domain : kBlockedDomains) {
				if (host.EndsWith(domain)) {
					int32 hostLen = host.Length();
					int32 domainLen = domain.Length();
					if (hostLen > domainLen && host.ByteAt(hostLen - domainLen - 1) == '.') {
						if (view)
							view->LoadURL("about:blank");
						return;
					}
				}
			}
		}
	}

	// Investigation Note:
	// "Private Window Context Menu" feature was investigated.
	// Implementing a "Open in Private Window" context menu option is not feasible
	// without modifying the BWebView/WebKit implementation to expose HitTest information
	// or context menu hooks, which are not available in the current API surface.

	PageUserData* userData = NULL;
	if (view)
		userData = static_cast<PageUserData*>(view->GetUserData());

	// HTTPS-Only Mode
	if (fAppSettings->GetValue(kSettingsKeyHttpsOnly, false)) {
		if (userData && userData->ExpectedUpgradedUrl() == url) {
			userData->SetHttpsUpgraded(true);
			userData->SetExpectedUpgradedUrl("");
		} else if (userData) {
			userData->SetHttpsUpgraded(false);
		}

		if (url.StartsWith("http://")) {
			BUrl newUrl(url);
			if (newUrl.IsValid()) {
				bool skipUpgrade = false;
				if (userData && userData->AllowedInsecureHost() == newUrl.Host()) {
					// User allowed this host to be insecure
					skipUpgrade = true;
				}

				if (!skipUpgrade) {
					newUrl.SetProtocol("https");
					if (newUrl.Port() == 80)
						newUrl.SetPort(443);

					BString httpsUrl = newUrl.UrlString();
					if (view) {
						if (userData)
							userData->SetExpectedUpgradedUrl(httpsUrl);
						view->LoadURL(httpsUrl);
					}
					return; // Abort this load, we started a new one
				}
			}
		}
	}

	// Apply per-site permissions
	bool allowJS = true;
	bool allowCookies = true;
	bool allowPopups = true;
	float zoom = 1.0;
	bool forceDesktop = false;
	BString customUserAgent;
	SitePermissionsManager::Instance()->CheckPermission(url.String(), allowJS, allowCookies, allowPopups, zoom, forceDesktop, customUserAgent);

	if (view && view->WebPage()) {
		BWebSettings* settings = view->WebPage()->Settings();
		if (settings) {
			settings->SetJavaScriptEnabled(allowJS);
			settings->SetCookiesEnabled(allowCookies);

			// Only apply global cache setting if we are NOT currently in a forced reload (bypass cache) state
			if (!fIsBypassingCache) {
				if (fAppSettings->GetValue(kSettingsKeyDisableCache, false)) {
					view->WebPage()->SetCacheModel(B_WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER);
				} else {
					view->WebPage()->SetCacheModel(B_WEBKIT_CACHE_MODEL_WEB_BROWSER);
				}
			}
			// Force Desktop
			if (forceDesktop) {
				settings->SetUserAgent("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.0 Safari/605.1.15");
			} else if (customUserAgent.Length() > 0) {
				settings->SetUserAgent(customUserAgent.String());
			} else {
				settings->SetUserAgent(NULL); // Reset to default
			}

			// Load Images (Global setting)
			bool loadImages = fAppSettings->GetValue(kSettingsKeyLoadImages, true);
			settings->SetLoadsImagesAutomatically(loadImages);
		}
	}

	if (view == CurrentWebView())
		fIsLoading = true;

	if (userData != NULL)
		userData->SetIsLoading(true);

	if (view != CurrentWebView()) {
		// Update the userData contents instead so the user sees
		// the correct URL when they switch back to that tab.
		if (userData != NULL && userData->URLInputContents().Length() == 0) {
			userData->SetURLInputContents(url);
		}
	}

	fURLInputGroup->SetText(url.String());

	BString status(B_TRANSLATE("Requesting %url"));
	status.ReplaceFirst("%url", url);
	view->WebPage()->SetStatusMessage(status);
}


void
BrowserWindow::LoadCommitted(const BString& url, BWebView* view)
{
	PageUserData* userData = static_cast<PageUserData*>(view->GetUserData());
	if (userData != NULL)
		userData->SetIsLoading(true);

	if (fLowRAMMode && view && view->WebPage()) {
		BString script = "var style = document.createElement('style');"
			"style.innerHTML = '* { animation: none !important; transition: none !important; }';"
			"document.head.appendChild(style);";
		view->WebPage()->ExecuteJavaScript(script);
	}

	if (view != CurrentWebView())
		return;

	fIsLoading = true;

	// This hook is invoked when the load is committed.
	fURLInputGroup->SetText(url.String());

	BString status(B_TRANSLATE("Loading %url"));
	status.ReplaceFirst("%url", url);
	view->WebPage()->SetStatusMessage(status);
}


void
BrowserWindow::LoadProgress(float progress, BWebView* view)
{
	if (view != CurrentWebView())
		return;

	if (progress < 100 && fLoadingProgressBar->IsHidden())
		_ShowProgressBar(true);
	else if (progress == 100 && !fLoadingProgressBar->IsHidden())
		_ShowProgressBar(false);
	fLoadingProgressBar->SetTo(progress);
}


void
BrowserWindow::LoadFailed(const BString& url, BWebView* view)
{
	if (fNetworkWindow) {
		BMessage msg(UPDATE_NETWORK_REQUEST);
		msg.AddString("url", url);
		msg.AddString("status", "Failed");
		fNetworkWindow->PostMessage(&msg);
	}

	if (fIsBypassingCache && view && view->WebPage()) {
		// Restore cache model on failure too
		if (!fAppSettings->GetValue(kSettingsKeyDisableCache, false)) {
			view->WebPage()->SetCacheModel(B_WEBKIT_CACHE_MODEL_WEB_BROWSER);
		}
		fIsBypassingCache = false;
	}

	PageUserData* userData = static_cast<PageUserData*>(view->GetUserData());
	if (userData != NULL)
		userData->SetIsLoading(false);

	if (view != CurrentWebView())
		return;

	fIsLoading = false;

	BString status(B_TRANSLATE_COMMENT("%url failed", "Loading URL failed. "
		"Don't translate variable %url."));
	status.ReplaceFirst("%url", url);

	if (userData != NULL && userData->IsHttpsUpgraded()) {
		status << " " << B_TRANSLATE("(HTTPS-Only mode)");

		BString text(B_TRANSLATE("The secure connection to %url failed."));
		text.ReplaceFirst("%url", url);
		text << "\n\n" << B_TRANSLATE("Do you want to try loading the unencrypted (insecure) version?");

		BAlert* alert = new BAlert(B_TRANSLATE("HTTPS-Only Mode"), text.String(),
			B_TRANSLATE("Cancel"), B_TRANSLATE("Load Insecurely"));

		if (alert->Go() == 1) {
			BString httpUrl = url;
			if (httpUrl.StartsWith("https://")) {
				httpUrl.ReplaceFirst("https://", "http://");
			} else {
				httpUrl.Prepend("http://");
			}

			// Allow this host
			BUrl u(httpUrl);
			userData->SetAllowedInsecureHost(u.Host());
			userData->SetHttpsUpgraded(false);
			userData->SetExpectedUpgradedUrl("");

			view->LoadURL(httpUrl);
			return;
		}
	}

	view->WebPage()->SetStatusMessage(status);
	_EnsureProgressBarHidden();
}


void
BrowserWindow::LoadFinished(const BString& url, BWebView* view)
{
	if (fNetworkWindow) {
		BMessage msg(UPDATE_NETWORK_REQUEST);
		msg.AddString("url", url);
		msg.AddString("status", "Finished");
		fNetworkWindow->PostMessage(&msg);
	}

	if (fIsBypassingCache && view && view->WebPage()) {
		// Restore cache model if we were bypassing, UNLESS disable cache setting is on
		if (!fAppSettings->GetValue(kSettingsKeyDisableCache, false)) {
			view->WebPage()->SetCacheModel(B_WEBKIT_CACHE_MODEL_WEB_BROWSER);
		}
		fIsBypassingCache = false;
	}

	// Check permissions for popups and dark mode injection
	bool allowJS = true;
	bool allowCookies = true;
	bool allowPopups = true;
	float zoom = 1.0;
	bool forceDesktop = false;
	BString customUserAgent;
	bool found = SitePermissionsManager::Instance()->CheckPermission(url.String(), allowJS, allowCookies, allowPopups, zoom, forceDesktop, customUserAgent);

	if (view) {
		// Apply Per-Site Zoom
		// Note: We only apply if found, or if we want to enforce 1.0 default if not found?
		// Usually if not found we let it remain what user set, OR we reset to 1.0?
		// The prompt says "persistence", which implies if I visit a new site it should be default.
		// If I visit a site I saved, it should be saved value.
		// If I changed it manually, it should save.
		// So if found, we apply 'zoom'. If not found, we apply 1.0?
		// Existing behavior resets zoom on navigation?
		// Let's assume if 'found', we apply. If not, we reset to 1.0.
		if (found) {
			view->SetZoomFactor(zoom);
		} else {
			view->SetZoomFactor(1.0);
		}
	}

	if (found && !allowPopups) {
		// Enforce No Popups via JS
		if (view && view->WebPage()) {
			view->WebPage()->ExecuteJavaScript(
				"window.open = function() { console.log('Popups blocked by WebPositive permissions'); return null; };");
		}
	}

	if (fDarkMode && view && view->WebPage()) {
		BString script = "if (typeof toggleDarkMode === 'undefined') {"
			"  toggleDarkMode = function(enable) {"
			"    if (enable) {"
			"      var style = document.createElement('style');"
			"      style.id = 'webpositive-dark-mode';"
			"      style.innerHTML = 'html { filter: invert(100%); } img, video { filter: invert(100%); }';"
			"      document.head.appendChild(style);"
			"    } else {"
			"      var style = document.getElementById('webpositive-dark-mode');"
			"      if (style) style.remove();"
			"    }"
			"  };"
			"}"
			"toggleDarkMode(true);";
		view->WebPage()->ExecuteJavaScript(script);
	}

	if (fReaderMode && view && view->WebPage()) {
		BString script = "if (typeof toggleReaderMode === 'undefined') {"
			"  toggleReaderMode = function(enable) {"
			"    var tags = ['nav', 'footer', 'aside', 'header', '.ads', '.sidebar'];"
			"    tags.forEach(function(tag) {"
			"      var elements = document.querySelectorAll(tag);"
			"      elements.forEach(function(el) {"
			"        el.style.display = enable ? 'none' : '';"
			"      });"
			"    });"
			"    document.body.style.maxWidth = enable ? '800px' : '';"
			"    document.body.style.margin = enable ? '0 auto' : '';"
			"    document.body.style.fontSize = enable ? '18px' : '';"
			"    document.body.style.lineHeight = enable ? '1.6' : '';"
			"  };"
			"}"
			"toggleReaderMode(true);";
		view->WebPage()->ExecuteJavaScript(script);
	}

	// Ad Blocking
	if (fAppSettings->GetValue(kSettingsKeyBlockAds, false) && view && view->WebPage()) {
		BString script = "var style = document.createElement('style');"
			"style.innerHTML = '"
			".ads, .ad, .advertisement, [id^=\"google_ads\"], [id^=\"div-gpt-ad\"], "
			".doubleclick, .ad-banner, .banner-ad, .sponsor "
			"{ display: none !important; }';"
			"document.head.appendChild(style);";
		view->WebPage()->ExecuteJavaScript(script);
	}

	PageUserData* userData = static_cast<PageUserData*>(view->GetUserData());
	if (userData != NULL) {
		userData->SetIsLoading(false);
		if (userData->IsDownloadRestart())
			userData->SetIsDownloadRestart(false);
	}

	if (view != CurrentWebView())
		return;

	fIsLoading = false;

	fURLInputGroup->SetText(url.String());

	BString status(B_TRANSLATE_COMMENT("%url finished", "Loading URL "
		"finished. Don't translate variable %url."));
	status.ReplaceFirst("%url", url);
	view->WebPage()->SetStatusMessage(status);
	_EnsureProgressBarHidden();

	NavigationCapabilitiesChanged(fBackButton->IsEnabled(),
		fForwardButton->IsEnabled(), false, view);

	int32 tabIndex = fTabManager->TabForView(view);
	if (tabIndex > 0 && strcmp(B_TRANSLATE("New tab"),
		fTabManager->TabLabel(tabIndex)) == 0)
			fTabManager->SetTabLabel(tabIndex, url);
}


void
BrowserWindow::MainDocumentError(const BString& failingURL,
	const BString& localizedDescription, BWebView* view)
{
	PageUserData* userData = static_cast<PageUserData*>(view->GetUserData());
	if (userData != NULL && userData->IsDownloadRestart()
		&& localizedDescription.FindFirst("interrupted") >= 0) {
		_ShutdownTab(fTabManager->TabForView(view));
		if (fTabManager->CountTabs() == 0)
			PostMessage(B_QUIT_REQUESTED);
		return;
	}

	// Make sure we show the page that contains the view.
	if (!_ShowPage(view))
		return;

	// Try delegating the URL to an external app instead.
	int32 at = failingURL.FindFirst(":");
	if (at > 0) {
		BString proto;
		failingURL.CopyInto(proto, 0, at);

		bool handled = false;

		for (unsigned int i = 0; i < sizeof(kHandledProtocols) / sizeof(char*);
				i++) {
			handled = (proto == kHandledProtocols[i]);
			if (handled)
				break;
		}

		if (!handled) {
			_SmartURLHandler(failingURL);
			return;
		}
	}

	BWebWindow::MainDocumentError(failingURL, localizedDescription, view);

	// Remove the failing URL from the browsing history.
	BrowsingHistory::DefaultInstance()->RemoveUrl(failingURL);
}


void
BrowserWindow::TitleChanged(const BString& title, BWebView* view)
{
	int32 tabIndex = fTabManager->TabForView(view);
	if (tabIndex < 0)
		return;

	fTabManager->SetTabLabel(tabIndex, title);

	if (view != CurrentWebView())
		return;

	_UpdateTitle(title);
}


void
BrowserWindow::IconReceived(const BBitmap* icon, BWebView* view)
{
	// The view may already be gone, since this notification arrives
	// asynchronously.
	if (!fTabManager->HasView(view))
		return;

	_SetPageIcon(view, icon);
}


void
BrowserWindow::ResizeRequested(float width, float height, BWebView* view)
{
	if (view != CurrentWebView())
		return;

	// Ignore request when there is more than one BWebView embedded.
	if (fTabManager->CountTabs() > 1)
		return;

	// Make sure the new frame is not larger than the screen frame minus
	// window decorator border.
	BScreen screen(this);
	BRect screenFrame = screen.Frame();
	BRect decoratorFrame = DecoratorFrame();
	BRect frame = Frame();

	screenFrame.left += decoratorFrame.left - frame.left;
	screenFrame.right += decoratorFrame.right - frame.right;
	screenFrame.top += decoratorFrame.top - frame.top;
	screenFrame.bottom += decoratorFrame.bottom - frame.bottom;

	width = min_c(width, screen.Frame().Width());
	height = min_c(height, screen.Frame().Height());

	frame.right = frame.left + width;
	frame.bottom = frame.top + height;

	// frame is now not larger than screenFrame, but may still be partly outside
	if (!screenFrame.Contains(frame)) {
		if (frame.left < screenFrame.left)
			frame.OffsetBy(screenFrame.left - frame.left, 0);
		else if (frame.right > screenFrame.right)
			frame.OffsetBy(screenFrame.right - frame.right, 0);
		if (frame.top < screenFrame.top)
			frame.OffsetBy(screenFrame.top - frame.top, 0);
		else if (frame.bottom > screenFrame.bottom)
			frame.OffsetBy(screenFrame.bottom - frame.bottom, 0);
	}

	MoveTo(frame.left, frame.top);
	ResizeTo(width, height);
}


void
BrowserWindow::SetToolBarsVisible(bool flag, BWebView* view)
{
	if (view != CurrentWebView())
		return;

	// Ignore request when there is more than one BWebView embedded.
	if (fTabManager->CountTabs() > 1)
		return;

	if (flag)
		fVisibleInterfaceElements |= INTERFACE_ELEMENT_NAVIGATION;
	else
		fVisibleInterfaceElements &= ~INTERFACE_ELEMENT_NAVIGATION;

	if (fInterfaceVisible) {
		fNavigationGroup->SetVisible(flag);

		if (fBookmarkBar != NULL) {
			if (flag) {
				if (fAppSettings->GetValue(kSettingsShowBookmarkBar, true)) {
					fBookmarkBar->Show();
					fBookmarkBarMenuItem->SetMarked(true);
				}
			} else {
				if (!fBookmarkBar->IsHidden()) {
					fBookmarkBar->Hide();
					fBookmarkBarMenuItem->SetMarked(false);
				}
			}
		}
	}
}


void
BrowserWindow::SetStatusBarVisible(bool flag, BWebView* view)
{
	if (view != CurrentWebView())
		return;

	// Ignore request when there is more than one BWebView embedded!
	if (fTabManager->CountTabs() > 1)
		return;

	if (flag)
		fVisibleInterfaceElements |= INTERFACE_ELEMENT_STATUS;
	else
		fVisibleInterfaceElements &= ~INTERFACE_ELEMENT_STATUS;

	if (fInterfaceVisible)
		fStatusGroup->SetVisible(flag); // Update status group visibility
}


void
BrowserWindow::SetMenuBarVisible(bool flag, BWebView* view)
{
	if (view != CurrentWebView())
		return;

	// Ignore request when there is more than one BWebView embedded.
	if (fTabManager->CountTabs() > 1)
		return;

	if (flag)
		fVisibleInterfaceElements |= INTERFACE_ELEMENT_MENU;
	else
		fVisibleInterfaceElements &= ~INTERFACE_ELEMENT_MENU;

#if !INTEGRATE_MENU_INTO_TAB_BAR
	if (fInterfaceVisible)
		fMenuGroup->SetVisible(flag);
#endif
}


void
BrowserWindow::SetResizable(bool flag, BWebView* view)
{
	if (view != CurrentWebView())
		return;

	// Ignore request when there is more than one BWebView embedded!
	if (fTabManager->CountTabs() > 1)
		return;

	if (flag)
		SetFlags(Flags() & ~B_NOT_RESIZABLE);
	else
		SetFlags(Flags() | B_NOT_RESIZABLE);
}


void
BrowserWindow::StatusChanged(const BString& statusText, BWebView* view)
{
	if (statusText.Compare("WebPositive:FormDirty:", 22) == 0) {
		fFormSafetyHelper->StatusChanged(statusText, view);
		// Do not show this status message in the UI
		return;
	}

	if (view != CurrentWebView())
		return;

	if (fStatusText)
		fStatusText->SetText(statusText.String());
}


void
BrowserWindow::NavigationCapabilitiesChanged(bool canGoBackward,
	bool canGoForward, bool canStop, BWebView* view)
{
	if (view != CurrentWebView())
		return;

	fBackButton->SetEnabled(canGoBackward);
	fForwardButton->SetEnabled(canGoForward);
	fStopButton->SetEnabled(canStop);

	fBackMenuItem->SetEnabled(canGoBackward);
	fForwardMenuItem->SetEnabled(canGoForward);
}


void
BrowserWindow::UpdateGlobalHistory(const BString& url)
{
	BrowsingHistory::DefaultInstance()->AddItem(BrowsingHistoryItem(url));

	BWebView* webView = CurrentWebView();
	if (webView != NULL)
		fURLInputGroup->SetText(webView->MainFrameURL());
}


bool
BrowserWindow::AuthenticationChallenge(BString message, BString& inOutUser,
	BString& inOutPassword, bool& inOutRememberCredentials,
	uint32 failureCount, BWebView* view)
{
	CredentialsStorage* persistentStorage
		= CredentialsStorage::PersistentInstance();
	CredentialsStorage* sessionStorage
		= CredentialsStorage::SessionInstance();

	// Build a stronger and more specific key for credential storage.
	BString keyString;

	if (view != NULL) {
		BUrl url(view->MainFrameURL());
		if (url.IsValid()) {
			// protocol://host:port
			BString proto(url.Protocol());
			BString host(url.Host());
			int32 port = url.Port();

			if (port >= 0) {
				keyString.SetToFormat("%s://%s:%" B_PRId32 ":",
					proto.String(), host.String(), port);
			} else {
				keyString.SetToFormat("%s://%s:",
					proto.String(), host.String());
			}
		}
	}

	// Append the realm/message last
	keyString << message;

	HashString key(keyString);

	// Try the new specific key first
	if (failureCount == 0) {
		if (persistentStorage->Contains(key)) {
			Credentials credentials = persistentStorage->GetCredentials(key);
			inOutUser = credentials.Username();
			inOutPassword = credentials.Password();
			return true;
		} else if (sessionStorage->Contains(key)) {
			Credentials credentials = sessionStorage->GetCredentials(key);
			inOutUser = credentials.Username();
			inOutPassword = credentials.Password();
			return true;
		}

		// Backward compatibility: Try the old less specific key
		BString legacyKeyString(message);
		if (view != NULL) {
			BUrl url(view->MainFrameURL());
			if (url.IsValid())
				legacyKeyString.Prepend(BString().SetToFormat("%s:", url.Host().String()));
		}
		HashString legacyKey(legacyKeyString);

		if (persistentStorage->Contains(legacyKey)) {
			Credentials credentials = persistentStorage->GetCredentials(legacyKey);
			inOutUser = credentials.Username();
			inOutPassword = credentials.Password();
			// Auto-migrate to new key
			persistentStorage->PutCredentials(key, credentials);
			return true;
		} else if (sessionStorage->Contains(legacyKey)) {
			Credentials credentials = sessionStorage->GetCredentials(legacyKey);
			inOutUser = credentials.Username();
			inOutPassword = credentials.Password();
			// Auto-migrate to new key
			sessionStorage->PutCredentials(key, credentials);
			return true;
		}
	}

	// Switch to the page for which this authentication is required.
	if (!_ShowPage(view))
		return false;

	AuthenticationPanel* panel = new AuthenticationPanel(Frame());
		// Panel auto-destructs.
	bool success = panel->getAuthentication(message, inOutUser, inOutPassword,
		inOutRememberCredentials, failureCount > 0, inOutUser, inOutPassword,
		&inOutRememberCredentials);
	if (success) {
		Credentials credentials(inOutUser, inOutPassword);
		if (inOutRememberCredentials)
			persistentStorage->PutCredentials(key, credentials);
		else
			sessionStorage->PutCredentials(key, credentials);
	}
	return success;
}


// #pragma mark - private


void
BrowserWindow::_UpdateTitle(const BString& title)
{
	BString windowTitle;

	if (title.Length() > 0)
		windowTitle = title;
	else {
		BWebView* webView = CurrentWebView();
		if (webView != NULL) {
			BString url = webView->MainFrameURL();
			int32 leafPos = url.FindLast('/');
			url.Remove(0, leafPos + 1);
			windowTitle = url;
		}
	}

	if (windowTitle.Length() > 0)
		windowTitle << " - ";
	windowTitle << kApplicationName;
	SetTitle(windowTitle.String());
}


void
BrowserWindow::_UpdateTabGroupVisibility()
{
	if (Lock()) {
		if (fInterfaceVisible)
			fTabGroup->SetVisible(_TabGroupShouldBeVisible());
		fTabManager->SetCloseButtonsAvailable(fTabManager->CountTabs() > 1);
		Unlock();
	}
}


bool
BrowserWindow::_TabGroupShouldBeVisible() const
{
	return (fShowTabsIfSinglePageOpen || fTabManager->CountTabs() > 1)
		&& (fVisibleInterfaceElements & INTERFACE_ELEMENT_TABS) != 0;
}


void
BrowserWindow::_ShutdownTab(int32 index)
{
	BView* view = fTabManager->ViewForTab(index);
	BWebView* webView = dynamic_cast<BWebView*>(view);

	if (webView != NULL) {
		BString url = webView->MainFrameURL();
		if (url.Length() > 0 && url != "about:blank") {
			ClosedTabInfo info;
			info.url = url;
			info.title = webView->MainFrameTitle();
			fClosedTabs.push_back(info);
			if (fClosedTabs.size() > 10)
				fClosedTabs.pop_front();
			_UpdateRecentlyClosedMenu();
		}
	}

	// If the tab to be removed is the current one, we must unset it *before*
	// removing it from the layout. Otherwise, SetCurrentWebView(NULL) might
	// try to capture a preview of a view that is no longer attached to the
	// window, resulting in graphical glitches or crashes.
	if (webView == CurrentWebView())
		SetCurrentWebView(NULL);

	view = fTabManager->RemoveTab(index);
	// webView pointer is still valid here, as RemoveTab only removed it from layout

	if (webView != NULL)
		webView->Shutdown();

	delete view;
}


void
BrowserWindow::_TabChanged(int32 index)
{
	SetCurrentWebView(dynamic_cast<BWebView*>(fTabManager->ViewForTab(index)));
}




void
BrowserWindow::_SetPageIcon(BWebView* view, const BBitmap* icon, bool save)
{
	PageUserData* userData = static_cast<PageUserData*>(view->GetUserData());
	if (userData == NULL) {
		userData = new PageUserData(NULL);
		view->SetUserData(userData);
	}
	// The PageUserData makes a copy of the icon, which we pass on to
	// the TabManager for display in the respective tab.
	userData->SetPageIcon(icon);

	if (save && icon) {
		_SaveFavicon(view->MainFrameURL(), icon);
	}

	fTabManager->SetTabIcon(view, userData->PageIcon());
	if (view == CurrentWebView())
		fURLInputGroup->SetPageIcon(icon);
}


class HistoryMenuBuilder {
public:
	HistoryMenuBuilder(BMenu* menu)
		:
		fMenu(menu)
	{
	}

	void AddItem(BMenuItem* newItem)
	{
		BString baseURLLabel = baseURL(BString(newItem->Label()));

		// Check if we already have a submenu for this base URL
		std::map<BString, BMenu*>::iterator subMenuIt
			= fSubMenus.find(baseURLLabel);
		if (subMenuIt != fSubMenus.end()) {
			subMenuIt->second->AddItem(newItem);
			return;
		}

		// Check if we have a single item waiting to be promoted
		std::map<BString, BMenuItem*>::iterator singleItemIt
			= fSingleItems.find(baseURLLabel);
		if (singleItemIt != fSingleItems.end()) {
			BMenuItem* oldItem = singleItemIt->second;

			// Remove old item from the main menu
			int32 index = fMenu->IndexOf(oldItem);
			fMenu->RemoveItem(oldItem);

			// Create new submenu
			BMenu* subMenu = new BMenu(baseURLLabel.String());
			subMenu->AddItem(oldItem);
			subMenu->AddItem(newItem);

			// Add common submenu for this base URL, clickable.
			BMessage* message = new BMessage(GOTO_URL);
			message->AddString("url", baseURLLabel.String());

			BMenuItem* subMenuItem = new BMenuItem(subMenu, message);
			if (index >= 0)
				fMenu->AddItem(subMenuItem, index);
			else
				fMenu->AddItem(subMenuItem);

			// Update maps
			fSingleItems.erase(singleItemIt);
			fSubMenus[baseURLLabel] = subMenu;
			return;
		}

		// Otherwise just add it
		fMenu->AddItem(newItem);
		fSingleItems[baseURLLabel] = newItem;
	}

private:
	BMenu* fMenu;
	std::map<BString, BMenu*> fSubMenus;
	std::map<BString, BMenuItem*> fSingleItems;
};


static void
addOrDeleteMenu(BMenu* menu, BMenu* toMenu)
{
	if (menu->CountItems() > 0)
		toMenu->AddItem(menu);
	else
		delete menu;
}


void
BrowserWindow::_UpdateHistoryMenu()
{
	BrowsingHistory* history = BrowsingHistory::DefaultInstance();
	if (!history->Lock())
		return;

	BDateTime todayStart = BDateTime::CurrentDateTime(B_LOCAL_TIME);
	todayStart.SetTime(BTime(0, 0, 0));

	if (history->Generation() == fLastHistoryGeneration
		&& todayStart.Date() == fLastHistoryMenuDate.Date()) {
		history->Unlock();
		return;
	}

	fLastHistoryGeneration = history->Generation();
	fLastHistoryMenuDate = todayStart;

	BMenuItem* menuItem;
	while ((menuItem = fHistoryMenu->RemoveItem(fHistoryMenuFixedItemCount)))
		delete menuItem;

	int32 count = history->CountItems();
	BMenuItem* clearHistoryItem = new BMenuItem(B_TRANSLATE("Clear history"),
		new BMessage(CLEAR_HISTORY));
	clearHistoryItem->SetEnabled(count > 0);
	fHistoryMenu->AddItem(clearHistoryItem);
	if (count == 0) {
		history->Unlock();
		return;
	}
	fHistoryMenu->AddSeparatorItem();

	BDateTime oneDayAgoStart = todayStart;
	oneDayAgoStart.Date().AddDays(-1);

	BDateTime twoDaysAgoStart = oneDayAgoStart;
	twoDaysAgoStart.Date().AddDays(-1);

	BDateTime threeDaysAgoStart = twoDaysAgoStart;
	threeDaysAgoStart.Date().AddDays(-1);

	BDateTime fourDaysAgoStart = threeDaysAgoStart;
	fourDaysAgoStart.Date().AddDays(-1);

	BDateTime fiveDaysAgoStart = fourDaysAgoStart;
	fiveDaysAgoStart.Date().AddDays(-1);

	BMenu* todayMenu = new BMenu(B_TRANSLATE("Today"));
	BMenu* yesterdayMenu = new BMenu(B_TRANSLATE("Yesterday"));
	BMenu* twoDaysAgoMenu = new BMenu(
		twoDaysAgoStart.Date().LongDayName().String());
	BMenu* threeDaysAgoMenu = new BMenu(
		threeDaysAgoStart.Date().LongDayName().String());
	BMenu* fourDaysAgoMenu = new BMenu(
		fourDaysAgoStart.Date().LongDayName().String());
	BMenu* fiveDaysAgoMenu = new BMenu(
		fiveDaysAgoStart.Date().LongDayName().String());
	BMenu* earlierMenu = new BMenu(B_TRANSLATE("Earlier"));

	HistoryMenuBuilder todayBuilder(todayMenu);
	HistoryMenuBuilder yesterdayBuilder(yesterdayMenu);
	HistoryMenuBuilder twoDaysAgoBuilder(twoDaysAgoMenu);
	HistoryMenuBuilder threeDaysAgoBuilder(threeDaysAgoMenu);
	HistoryMenuBuilder fourDaysAgoBuilder(fourDaysAgoMenu);
	HistoryMenuBuilder fiveDaysAgoBuilder(fiveDaysAgoMenu);
	HistoryMenuBuilder earlierBuilder(earlierMenu);

	int32 start = std::max((int32)0, count - 100);
	for (int32 i = start; i < count; i++) {
		BrowsingHistoryItem historyItem = history->HistoryItemAt(i);
		BMessage* message = new BMessage(GOTO_URL);
		message->AddString("url", historyItem.URL().String());

		BString truncatedUrl(historyItem.URL());
		be_plain_font->TruncateString(&truncatedUrl, B_TRUNCATE_END, 480);
		menuItem = new BMenuItem(truncatedUrl, message);

		if (historyItem.DateTime() < fiveDaysAgoStart)
			earlierBuilder.AddItem(menuItem);
		else if (historyItem.DateTime() < fourDaysAgoStart)
			fiveDaysAgoBuilder.AddItem(menuItem);
		else if (historyItem.DateTime() < threeDaysAgoStart)
			fourDaysAgoBuilder.AddItem(menuItem);
		else if (historyItem.DateTime() < twoDaysAgoStart)
			threeDaysAgoBuilder.AddItem(menuItem);
		else if (historyItem.DateTime() < oneDayAgoStart)
			twoDaysAgoBuilder.AddItem(menuItem);
		else if (historyItem.DateTime() < todayStart)
			yesterdayBuilder.AddItem(menuItem);
		else
			todayBuilder.AddItem(menuItem);
	}
	history->Unlock();

	addOrDeleteMenu(todayMenu, fHistoryMenu);
	addOrDeleteMenu(yesterdayMenu, fHistoryMenu);
	addOrDeleteMenu(twoDaysAgoMenu, fHistoryMenu);
	addOrDeleteMenu(threeDaysAgoMenu, fHistoryMenu);
	addOrDeleteMenu(fourDaysAgoMenu, fHistoryMenu);
	addOrDeleteMenu(fiveDaysAgoMenu, fHistoryMenu);
	addOrDeleteMenu(earlierMenu, fHistoryMenu);
}


void
BrowserWindow::_UpdateClipboardItems()
{
	BTextView* focusTextView = dynamic_cast<BTextView*>(CurrentFocus());
	if (focusTextView != NULL) {
		int32 selectionStart;
		int32 selectionEnd;
		focusTextView->GetSelection(&selectionStart, &selectionEnd);
		bool hasSelection = selectionStart < selectionEnd;
		bool canPaste = false;
		// A BTextView has the focus.
		if (be_clipboard->Lock()) {
			BMessage* data = be_clipboard->Data();
			if (data != NULL)
				canPaste = data->HasData("text/plain", B_MIME_TYPE);
			be_clipboard->Unlock();
		}
		fCutMenuItem->SetEnabled(hasSelection);
		fCopyMenuItem->SetEnabled(hasSelection);
		fPasteMenuItem->SetEnabled(canPaste);
	} else if (CurrentWebView() != NULL) {
		// Trigger update of the clipboard items, even if the
		// BWebView doesn't have focus, we'll dispatch these message
		// there anyway. This works so fast that the user can never see
		// the wrong enabled state when the menu opens until the result
		// message arrives. The initial state needs to be enabled, since
		// standard shortcut handling is always wrapped inside MenusBeginning()
		// and MenusEnded(), and since we update items asynchronously, we need
		// to have them enabled to begin with.
		fCutMenuItem->SetEnabled(true);
		fCopyMenuItem->SetEnabled(true);
		fPasteMenuItem->SetEnabled(true);

		CurrentWebView()->WebPage()->SendEditingCapabilities();
	}
}


bool
BrowserWindow::_ShowPage(BWebView* view)
{
	if (view != CurrentWebView()) {
		int32 tabIndex = fTabManager->TabForView(view);
		if (tabIndex < 0) {
			// Page seems to be gone already?
			return false;
		}
		fTabManager->SelectTab(tabIndex);
		_TabChanged(tabIndex);
		UpdateIfNeeded();
	}
	return true;
}


void
BrowserWindow::_ResizeToScreen()
{
	BScreen screen(this);
	MoveTo(0, 0);
	ResizeTo(screen.Frame().Width(), screen.Frame().Height());
}


void
BrowserWindow::_SetAutoHideInterfaceInFullscreen(bool doIt)
{
	if (fAutoHideInterfaceInFullscreenMode == doIt)
		return;

	fAutoHideInterfaceInFullscreenMode = doIt;
	if (fAppSettings->GetValue(kSettingsKeyAutoHideInterfaceInFullscreenMode,
			doIt) != doIt) {
		fAppSettings->SetValue(kSettingsKeyAutoHideInterfaceInFullscreenMode,
			doIt);
	}

	if (fAutoHideInterfaceInFullscreenMode) {
		BMessage message(CHECK_AUTO_HIDE_INTERFACE);
		fPulseRunner.reset(new BMessageRunner(BMessenger(this), &message, 300000));
	} else {
		fPulseRunner.reset();
		_ShowInterface(true);
	}
}


void
BrowserWindow::_CheckAutoHideInterface()
{
	// Bookmark Bar auto-hide logic (if enabled and NOT fullscreen)
	if (!fIsFullscreen && fAutoHideBookmarkBar && fBookmarkBar) {
		if (fBookmarkBar->IsHidden()) {
			// Show if mouse at top (e.g. over menu/tabs/nav)
			// Navigation group bottom is a good threshold.
			if (fLastMousePos.y <= fNavigationGroup->Frame().bottom)
				fBookmarkBar->Show();
		} else {
			// Hide if mouse moves away
			if (fLastMousePos.y > fBookmarkBar->Frame().bottom + 10)
				fBookmarkBar->Hide();
		}
	}

	if (!fIsFullscreen || !fAutoHideInterfaceInFullscreenMode
		|| (CurrentWebView() != NULL && !CurrentWebView()->IsFocus())) {
		return;
	}

	if (fLastMousePos.y == 0)
		_ShowInterface(true);
	else if (fNavigationGroup->IsVisible()
		&& fLastMousePos.y > fNavigationGroup->Frame().bottom
		&& system_time() - fLastMouseMovedTime > 1000000) {
		// NOTE: Do not re-use navigationGroupBottom in the above
		// check, since we only want to hide the interface when it is visible.
		_ShowInterface(false);
	}
}


void
BrowserWindow::_ShowInterface(bool show)
{
	if (fInterfaceVisible == show)
		return;

	fInterfaceVisible = show;

	if (show) {
#if !INTEGRATE_MENU_INTO_TAB_BAR
		fMenuGroup->SetVisible(
			(fVisibleInterfaceElements & INTERFACE_ELEMENT_MENU) != 0);
#endif
		fTabGroup->SetVisible(_TabGroupShouldBeVisible());
		fNavigationGroup->SetVisible(
			(fVisibleInterfaceElements & INTERFACE_ELEMENT_NAVIGATION) != 0);
		fStatusGroup->SetVisible(
			(fVisibleInterfaceElements & INTERFACE_ELEMENT_STATUS) != 0);
	} else {
		fMenuGroup->SetVisible(false);
		fTabGroup->SetVisible(false);
		fNavigationGroup->SetVisible(false);
		fStatusGroup->SetVisible(false);
	}

	_EnsureProgressBarHidden();
}


void
BrowserWindow::_ShowProgressBar(bool show)
{
	if (show) {
		if (!fStatusGroup->IsVisible() && (fVisibleInterfaceElements
			& INTERFACE_ELEMENT_STATUS) != 0)
				fStatusGroup->SetVisible(true);
		fLoadingProgressBar->Show();
	} else {
		if (!fInterfaceVisible)
			fStatusGroup->SetVisible(false);

		_EnsureProgressBarHidden();
	}
}


void
BrowserWindow::_EnsureProgressBarHidden()
{
	if (!fLoadingProgressBar->IsHidden())
		fLoadingProgressBar->Hide();
}


void
BrowserWindow::_InvokeButtonVisibly(BButton* button)
{
	fButtonResetRunner.reset();

	button->SetValue(B_CONTROL_ON);
	// UpdateIfNeeded(); // Usually handled by window loop, removing unless critical
	button->Invoke();

	BMessage message(RESET_BUTTON_STATE);
	message.AddPointer("button", button);
	fButtonResetRunner.reset(new BMessageRunner(BMessenger(this), &message, 50000, 1));
}


BString
BrowserWindow::_NewTabURL(bool isNewWindow) const
{
	BString url;
	uint32 policy = isNewWindow ? fNewWindowPolicy : fNewTabPolicy;
	// Implement new page policy
	switch (policy) {
		case OpenStartPage:
			url = fStartPageURL;
			break;
		case OpenSearchPage:
			url.SetTo(fSearchPageURL);
			url.ReplaceAll("%s", "");
			break;
		case CloneCurrentPage:
			if (CurrentWebView() != NULL)
				url = CurrentWebView()->MainFrameURL();
			break;
		case OpenBlankPage:
		default:
			break;
	}
	return url;
}


void
BrowserWindow::_VisitURL(const BString& url)
{
	// fURLInputGroup->TextView()->SetText(url);
	CurrentWebView()->LoadURL(url.String());
}


/*! \brief "smart" parser for user-entered URLs

	We try to be flexible in what we accept as a valid URL. The protocol may
	be missing, or something we can't handle (in that case we run the matching
	app). If all attempts to make sense of the input fail, we make a search
	engine query for it.
 */
void
BrowserWindow::_SmartURLHandler(const BString& url)
{
	BString outURL;
	URLHandler::Action action = URLHandler::CheckURL(url, outURL, fSearchPageURL);

	switch (action) {
		case URLHandler::LOAD_URL:
			_VisitURL(outURL);
			break;
		case URLHandler::LAUNCH_APP:
			// Handled in CheckURL
			break;
		case URLHandler::DO_NOTHING:
			break;
	}
}




void
BrowserWindow::_ShowBookmarkBar(bool show)
{
	// It is not allowed to show the bookmark bar when it is empty
	if (show && (fBookmarkBar == NULL || fBookmarkBar->CountItems() <= 1))
	{
		fBookmarkBarMenuItem->SetMarked(false);
		return;
	}

	fBookmarkBarMenuItem->SetMarked(show);

	if (fBookmarkBar == NULL || fBookmarkBar->IsHidden() != show)
		return;

	fAppSettings->SetValue(kSettingsShowBookmarkBar, show);

	if (show)
		fBookmarkBar->Show();
	else
		fBookmarkBar->Hide();
}


void
BrowserWindow::_ReopenClosedTab()
{
	if (fClosedTabs.empty())
		return;

	ClosedTabInfo info = fClosedTabs.back();
	fClosedTabs.pop_back();
	_UpdateRecentlyClosedMenu();

	CreateNewTab(info.url, true);
}


void
BrowserWindow::_UpdateRecentlyClosedMenu()
{
	bool hasClosedTabs = !fClosedTabs.empty();
	if (fReopenClosedTabMenuItem != NULL)
		fReopenClosedTabMenuItem->SetEnabled(hasClosedTabs);

	if (fRecentlyClosedMenu != NULL) {
		fRecentlyClosedMenu->RemoveItems(0, fRecentlyClosedMenu->CountItems(), true);

		for (int32 i = (int32)fClosedTabs.size() - 1; i >= 0; i--) {
			const ClosedTabInfo& info = fClosedTabs[i];
			BMessage* msg = new BMessage(REOPEN_CLOSED_TAB_WITH_INDEX);
			msg->AddInt32("index", i);

			BString label = info.title;
			if (label.IsEmpty()) label = info.url;
			if (label.Length() > 50) {
				label.Truncate(50);
				label << B_UTF8_ELLIPSIS;
			}

			fRecentlyClosedMenu->AddItem(new BMenuItem(label, msg));
		}

		fRecentlyClosedMenu->SetEnabled(hasClosedTabs);
	}
}


void
BrowserWindow::_CheckMemoryPressure()
{
	if (fLowRAMMode) {
		_DiscardBackgroundTabs();
		return;
	}

	system_info info;
	if (get_system_info(&info) == B_OK) {
		uint64 usedPages = info.used_pages;
		uint64 maxPages = info.max_pages;
		// If more than 90% memory used
		if (usedPages > (uint64)(maxPages * 0.9)) {
			_DiscardBackgroundTabs();
		}
	}
}


void
BrowserWindow::_DiscardBackgroundTabs()
{
	BWebView* current = CurrentWebView();
	int32 count = fTabManager->CountTabs();
	for (int32 i = 0; i < count; i++) {
		BView* view = fTabManager->ViewForTab(i);
		BWebView* webView = dynamic_cast<BWebView*>(view);
		if (webView && webView != current) {
			PageUserData* userData = static_cast<PageUserData*>(webView->GetUserData());
			if (userData && !userData->IsDiscarded() && !userData->IsLazy() && !userData->IsLoading()) {
				// Save URL
				BString url = webView->MainFrameURL();
				if (url.Length() > 0 && url != "about:blank") {
					userData->SetPendingURL(url);
					userData->SetIsDiscarded(true);
					webView->LoadURL("about:blank");
				}
			}
		}
	}
}


void
BrowserWindow::_UpdateToolbarPlacement()
{
	BLayout* layout = GetLayout();
	// Check if navigation group is already in the layout
	int32 index = layout->IndexOfItem(fNavigationGroup);
	if (index >= 0)
		layout->RemoveItem(fNavigationGroup);

	if (fToolbarBottom) {
		// Add before status bar (last item)
		int32 count = layout->CountItems();
		layout->AddItem(fNavigationGroup, count - 1);
	} else {
		// Add after tabs.
		// Layout items:
		// 0: Menu (if not integrated) or Tabs (if integrated)
		// 1: Tabs (if not integrated)
		// ...
		// We want to insert after Tab Group.
		int32 tabIndex = layout->IndexOfItem(fTabGroup);
		int32 insertIndex = (tabIndex >= 0) ? tabIndex + 1 : 2;
		layout->AddItem(fNavigationGroup, insertIndex);
	}
}


status_t
BrowserWindow::_GetFaviconPath(const BString& url, BPath& path)
{
	if (url.Length() == 0)
		return B_BAD_VALUE;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append(kApplicationName);
	path.Append("Favicons");

	if (create_directory(path.Path(), 0777) != B_OK)
		return B_ERROR;

	BUrl parsedUrl(url);
	if (!parsedUrl.IsValid() || parsedUrl.Host().Length() == 0)
		return B_BAD_VALUE;

	BString filename(parsedUrl.Host());
	// Sanitize filename
	filename.ReplaceAll('/', '_');
	filename.ReplaceAll(':', '_');

	path.Append(filename.String());
	return B_OK;
}


void
BrowserWindow::_SaveFavicon(const BString& url, const BBitmap* icon)
{
	if (icon == NULL || fIsPrivate)
		return;

	BPath path;
	if (_GetFaviconPath(url, path) != B_OK)
		return;

	// Use clone + ImportBits logic instead of new BBitmap(icon)
	BBitmap* saveIcon = new BBitmap(icon->Bounds(), B_RGBA32);
	if (saveIcon->ImportBits(icon->Bits(), icon->BitsLength(), icon->BytesPerRow(), 0, icon->ColorSpace()) != B_OK) {
		delete saveIcon;
		return;
	}

	FaviconSaveParams* params = new(std::nothrow) FaviconSaveParams;
	if (params == NULL) {
		delete saveIcon;
		return;
	}

	params->path = path;
	params->icon = saveIcon;

	thread_id thread = spawn_thread(_SaveFaviconThread, "Save Favicon",
		B_LOW_PRIORITY, params);
	if (thread >= 0) {
		if (resume_thread(thread) != B_OK) {
			kill_thread(thread);
			delete saveIcon;
			delete params;
		}
	} else {
		delete saveIcon;
		delete params;
	}
}


void
BrowserWindow::_LoadFavicon(const BString& url, BWebView* view)
{
	if (view == NULL) return;

	// Don't overwrite existing icon
	PageUserData* userData = static_cast<PageUserData*>(view->GetUserData());
	if (userData && userData->PageIcon())
		return;

	BPath path;
	if (_GetFaviconPath(url, path) != B_OK)
		return;

	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() == B_OK) {
		int32 width, height;
		if (file.Read(&width, sizeof(width)) == sizeof(width) &&
			file.Read(&height, sizeof(height)) == sizeof(height)) {

			if (width > 0 && width < 256 && height > 0 && height < 256) {
				// Verify file size matches expected packed size
				off_t size;
				file.GetSize(&size);
				off_t expectedSize = sizeof(width) + sizeof(height) + (off_t)width * height * 4;

				if (size == expectedSize) {
					BBitmap* icon = new BBitmap(BRect(0, 0, width - 1, height - 1), B_RGBA32);
					if (icon->InitCheck() == B_OK) {
						int32 bytesPerRow = icon->BytesPerRow();
						int32 rowLen = width * 4;
						uint8* bits = (uint8*)icon->Bits();
						bool success = true;
						for (int32 i = 0; i < height; i++) {
							if (file.Read(bits + (i * bytesPerRow), rowLen) != rowLen) {
								success = false;
								break;
							}
						}
						if (success) {
							_SetPageIcon(view, icon, false); // Don't save back to disk
						}
					}
					delete icon;
				} else {
					// Invalid size or legacy file with padding. Discard.
					// We can't safely read legacy files because we don't know the original padding.
					// Deleting the file forces a fresh fetch next time.
					file.Unset();
					BEntry entry(path.Path());
					entry.Remove();
				}
			}
		}
	}
}
