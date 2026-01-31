/*
 * Copyright 2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "URLInputGroup.h"

#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Clipboard.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <LayoutUtils.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Resources.h>
#include <SeparatorView.h>
#include <TextView.h>
#include <Window.h>

#include <stdio.h>
#include <stdlib.h>

#include "BaseURL.h"
#include "BrowserWindow.h"
#include "BrowsingHistory.h"
#include "IconButton.h"
#include "PageUserData.h"
#include "SettingsKeys.h"
#include "IconUtils.h"
#include "TextViewCompleter.h"
#include "WebView.h"
#include "WebWindow.h"

#include "support/WebConstants.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "URL Bar"

const uint32 kMsgSearchButtonClick = 'sbcl';

class URLChoice : public BAutoCompleter::Choice {
public:
	URLChoice(const BString& choiceText, const BString& displayText,
			int32 matchPos, int32 matchLen, int32 priority)
		:
		BAutoCompleter::Choice(choiceText, displayText, matchPos, matchLen),
		fPriority(priority)
	{
	}

	bool operator<(const URLChoice& other) const
	{
		if (fPriority > other.fPriority)
			return true;
		return DisplayText() < other.DisplayText();
	}

	bool operator==(const URLChoice& other) const
	{
		return fPriority == other.fPriority
			&& DisplayText() < other.DisplayText();
	}

private:
	int32 fPriority;
};


class BrowsingHistoryChoiceModel : public BAutoCompleter::ChoiceModel {
	virtual void FetchChoicesFor(const BString& pattern)
	{
		int32 count = CountChoices();
		for (int32 i = 0; i < count; i++) {
			delete reinterpret_cast<BAutoCompleter::Choice*>(
				fChoices.ItemAtFast(i));
		}
		fChoices.MakeEmpty();

		// Search through BrowsingHistory for any matches.
		BrowsingHistory* history = BrowsingHistory::DefaultInstance();
		if (!history->Lock())
			return;

		BString lastBaseURL;
		int32 priority = INT_MAX;

		const int32 kMaxChoices = 50;
		count = history->CountItems();
		for (int32 i = count - 1; i >= 0; i--) {
			const BrowsingHistoryItem* item = history->HistoryItemAt(i);
			if (item == NULL)
				continue;
			const BString& choiceText = item->URL();
			int32 matchPos = choiceText.IFindFirst(pattern);
			if (matchPos < 0)
				continue;
			if (lastBaseURL.Length() > 0
				&& choiceText.FindFirst(lastBaseURL) >= 0) {
				priority--;
			} else
				priority = INT_MAX;
			lastBaseURL = baseURL(choiceText);
			fChoices.AddItem(new URLChoice(choiceText,
				choiceText, matchPos, pattern.Length(), priority));

			if (fChoices.CountItems() >= kMaxChoices)
				break;
		}

		history->Unlock();

		fChoices.SortItems(_CompareChoices);
	}

	virtual int32 CountChoices() const
	{
		return fChoices.CountItems();
	}

	virtual const BAutoCompleter::Choice* ChoiceAt(int32 index) const
	{
		return reinterpret_cast<BAutoCompleter::Choice*>(
			fChoices.ItemAt(index));
	}

	static int _CompareChoices(const void* a, const void* b)
	{
		const URLChoice* aChoice
			= *reinterpret_cast<const URLChoice* const *>(a);
		const URLChoice* bChoice
			= *reinterpret_cast<const URLChoice* const *>(b);
		if (*aChoice < *bChoice)
			return -1;
		else if (*aChoice == *bChoice)
			return 0;
		return 1;
	}

private:
	BList fChoices;
};


// #pragma mark - URLTextView


static const float kHorizontalTextRectInset = 4.0;


class URLInputGroup::URLTextView : public BTextView {
private:
	static const uint32 MSG_CLEAR = 'cler';
	static const uint32 PASTE_AND_GO = 'psgo';

public:
								URLTextView(URLInputGroup* parent);
	virtual						~URLTextView();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				MouseDown(BPoint where);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				MakeFocus(bool focused = true);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();

			void				SetUpdateAutoCompleterChoices(bool update);

protected:
	virtual	void				InsertText(const char* inText, int32 inLength,
									int32 inOffset,
									const text_run_array* inRuns);
	virtual	void				DeleteText(int32 fromOffset, int32 toOffset);

private:
			URLInputGroup*		fURLInputGroup;
			TextViewCompleter*	fURLAutoCompleter;
			bool				fUpdateAutoCompleterChoices;
};


URLInputGroup::URLTextView::URLTextView(URLInputGroup* parent)
	:
	BTextView("url"),
	fURLInputGroup(parent),
	fURLAutoCompleter(new TextViewCompleter(this,
		new BrowsingHistoryChoiceModel())),
	fUpdateAutoCompleterChoices(true)
{
	MakeResizable(true);
	SetStylable(true);
	SetInsets(be_control_look->DefaultLabelSpacing(), 2, 0, 2);
	fURLAutoCompleter->SetModificationsReported(true);
}


URLInputGroup::URLTextView::~URLTextView()
{
	delete fURLAutoCompleter;
}


void
URLInputGroup::URLTextView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_CLEAR:
			SetText("");
			break;

		case PASTE_AND_GO:
		{
			BMessage paste(B_PASTE);
			BTextView::MessageReceived(&paste);
			fURLInputGroup->GoButton()->Invoke();
			break;
		}

		default:
			BTextView::MessageReceived(message);
			break;
	}
}


void
URLInputGroup::URLTextView::MouseDown(BPoint where)
{
	bool wasFocus = IsFocus();
	if (!wasFocus)
		MakeFocus(true);

	int32 buttons;
	if (Window()->CurrentMessage()->FindInt32("buttons", &buttons) != B_OK)
		buttons = B_PRIMARY_MOUSE_BUTTON;

	if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
		// Display context menu
		int32 selectionStart;
		int32 selectionEnd;
		GetSelection(&selectionStart, &selectionEnd);
		bool canCutOrCopy = selectionEnd > selectionStart;

		bool canPaste = false;
		if (be_clipboard->Lock()) {
			if (BMessage* data = be_clipboard->Data())
				canPaste = data->HasData("text/plain", B_MIME_TYPE);
			be_clipboard->Unlock();
		}

		BMenuItem* cutItem = new BMenuItem(B_TRANSLATE("Cut"),
			new BMessage(B_CUT));
		BMenuItem* copyItem = new BMenuItem(B_TRANSLATE("Copy"),
			new BMessage(B_COPY));
		BMenuItem* pasteItem = new BMenuItem(B_TRANSLATE("Paste"),
			new BMessage(B_PASTE));
		BMenuItem* pasteAndGoItem = new BMenuItem(B_TRANSLATE("Paste and go"),
			new BMessage(PASTE_AND_GO));
		BMenuItem* clearItem = new BMenuItem(B_TRANSLATE("Clear"),
			new BMessage(MSG_CLEAR));
		cutItem->SetEnabled(canCutOrCopy);
		copyItem->SetEnabled(canCutOrCopy);
		pasteItem->SetEnabled(canPaste);
		pasteAndGoItem->SetEnabled(canPaste);
		clearItem->SetEnabled(strlen(Text()) > 0);

		BPopUpMenu* menu = new BPopUpMenu("url context");
		menu->AddItem(cutItem);
		menu->AddItem(copyItem);
		menu->AddItem(pasteItem);
		menu->AddItem(pasteAndGoItem);
		menu->AddItem(clearItem);

		menu->SetTargetForItems(this);
		menu->Go(ConvertToScreen(where), true, true, true);
		return;
	}

	// Only pass through to base class if we already have focus.
	if (!wasFocus)
		return;

	BTextView::MouseDown(where);
}


void
URLInputGroup::URLTextView::KeyDown(const char* bytes, int32 numBytes)
{
	switch (bytes[0]) {
		case B_TAB:
			BView::KeyDown(bytes, numBytes);
			break;

		case B_ESCAPE:
			// Text already unlocked && replaced in BrowserWindow,
			// now select it.
			SelectAll();
			break;

		case B_RETURN:
			// Don't let this through to the text view.
			break;

		default:
		{
			BString currentText = Text();
			BTextView::KeyDown(bytes, numBytes);
			// Lock the URL input if it was modified
			if (!fURLInputGroup->IsURLInputLocked()
				&& Text() != currentText)
				fURLInputGroup->LockURLInput();
			break;
		}
	}
}

void
URLInputGroup::URLTextView::MakeFocus(bool focus)
{
	// Unlock the URL input if focus was lost.
	if (!focus)
		fURLInputGroup->LockURLInput(false);

	if (focus == IsFocus())
		return;

	BTextView::MakeFocus(focus);

	if (focus)
		SelectAll();

	fURLInputGroup->Invalidate();
}


BSize
URLInputGroup::URLTextView::MinSize()
{
	BSize min;
	min.height = ceilf(LineHeight(0) + kHorizontalTextRectInset);
		// we always add at least one pixel vertical inset top/bottom for
		// the text rect.
	min.width = min.height * 3;
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), min);
}


BSize
URLInputGroup::URLTextView::MaxSize()
{
	BSize max(MinSize());
	max.width = B_SIZE_UNLIMITED;
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), max);
}


void
URLInputGroup::URLTextView::SetUpdateAutoCompleterChoices(bool update)
{
	fUpdateAutoCompleterChoices = update;
}


void
URLInputGroup::URLTextView::InsertText(const char* inText, int32 inLength,
	int32 inOffset, const text_run_array* inRuns)
{
	// Filter all line breaks, note that inText is not terminated.
	if (inLength == 1) {
		if (*inText == '\n' || *inText == '\r')
			BTextView::InsertText(" ", 1, inOffset, inRuns);
		else
			BTextView::InsertText(inText, 1, inOffset, inRuns);
	} else {
		BString filteredText(inText, inLength);
		filteredText.ReplaceAll('\n', ' ');
		filteredText.ReplaceAll('\r', ' ');
		BTextView::InsertText(filteredText.String(), inLength, inOffset,
			inRuns);
	}

	// Make the base URL part bold.
	BString text(Text(), TextLength());
	int32 baseUrlStart = text.FindFirst("://");
	if (baseUrlStart >= 0)
		baseUrlStart += 3;
	else
		baseUrlStart = 0;
	int32 baseUrlEnd = text.FindFirst("/", baseUrlStart);
	if (baseUrlEnd < 0)
		baseUrlEnd = TextLength();

	BFont font;
	GetFont(&font);
	const rgb_color hostColor = ui_color(B_DOCUMENT_TEXT_COLOR);
	const rgb_color urlColor = tint_color(hostColor,
		(hostColor.IsDark() ? B_LIGHTEN_1_TINT : B_DARKEN_1_TINT));
	if (baseUrlStart > 0)
		SetFontAndColor(0, baseUrlStart, &font, B_FONT_ALL, &urlColor);
	if (baseUrlEnd > baseUrlStart) {
		font.SetFace(B_BOLD_FACE);
		SetFontAndColor(baseUrlStart, baseUrlEnd, &font, B_FONT_ALL,
			&hostColor);
	}
	if (baseUrlEnd < TextLength()) {
		font.SetFace(B_REGULAR_FACE);
		SetFontAndColor(baseUrlEnd, TextLength(), &font, B_FONT_ALL,
			&urlColor);
	}

	fURLAutoCompleter->TextModified(fUpdateAutoCompleterChoices);
}


void
URLInputGroup::URLTextView::DeleteText(int32 fromOffset, int32 toOffset)
{
	BTextView::DeleteText(fromOffset, toOffset);

	fURLAutoCompleter->TextModified(fUpdateAutoCompleterChoices);
}


const unsigned char kPlaceholderIcon[] = {
	0x6e, 0x63, 0x69, 0x66, 0x04, 0x04, 0x00, 0x66, 0x03, 0x00, 0x3f, 0x80,
	0x02, 0x00, 0x06, 0x02, 0x00, 0x00, 0x00, 0x3d, 0xa6, 0x64, 0xc2, 0x19,
	0x98, 0x00, 0x00, 0x00, 0x4d, 0xce, 0x64, 0x49, 0xac, 0xcc, 0x00, 0xab,
	0xd5, 0xff, 0xff, 0x00, 0x6c, 0xd9, 0x02, 0x00, 0x06, 0x02, 0x00, 0x00,
	0x00, 0x3d, 0x26, 0x64, 0xc2, 0x19, 0x98, 0x00, 0x00, 0x00, 0x4d, 0xce,
	0x64, 0x49, 0xac, 0xcc, 0x00, 0x80, 0xff, 0x80, 0xff, 0x00, 0xb2, 0x00,
	0x04, 0x02, 0x04, 0x34, 0x22, 0xbd, 0x9b, 0x22, 0xb8, 0x53, 0x22, 0x28,
	0x2e, 0x28, 0xb5, 0xef, 0x28, 0xbb, 0x37, 0x34, 0x3a, 0xb8, 0x53, 0x3a,
	0xbd, 0x9b, 0x3a, 0x40, 0x2e, 0x40, 0xbb, 0x37, 0x40, 0xb5, 0xef, 0x02,
	0x08, 0xbe, 0xb6, 0xb4, 0xac, 0xc1, 0x46, 0xb4, 0xac, 0xbc, 0x25, 0xb4,
	0xac, 0xb8, 0x09, 0xb7, 0x35, 0xb9, 0xcf, 0xb5, 0xa0, 0xb8, 0x05, 0xbe,
	0xb6, 0x35, 0xc2, 0xe5, 0x35, 0xbe, 0xb6, 0x35, 0xc5, 0x68, 0xb8, 0x09,
	0xc6, 0x36, 0xb8, 0x09, 0xc6, 0x36, 0xb9, 0xcf, 0xc7, 0xca, 0xbe, 0xb6,
	0xc8, 0xc1, 0xbc, 0x25, 0xc8, 0xc1, 0xc1, 0xb3, 0xc8, 0xc1, 0xc6, 0x3c,
	0xc5, 0x5b, 0xc4, 0x65, 0xc7, 0x70, 0xc6, 0x3e, 0xbe, 0xb6, 0xc2, 0x0f,
	0xba, 0x87, 0xc2, 0x0f, 0xbe, 0xb6, 0xc2, 0x0f, 0xb8, 0x05, 0xc5, 0x64,
	0xb7, 0x37, 0xc5, 0x64, 0xb7, 0x37, 0xc3, 0x9e, 0xb5, 0xa2, 0x02, 0x04,
	0xb8, 0x09, 0xb7, 0x35, 0xb8, 0x05, 0xbe, 0xb6, 0xb5, 0xf8, 0xb9, 0x0c,
	0xb4, 0xac, 0xbe, 0xb6, 0xb4, 0xac, 0xbb, 0xba, 0xb4, 0xac, 0xc1, 0xb1,
	0xb8, 0x09, 0xc6, 0x36, 0xb5, 0xf8, 0xc4, 0x5e, 0xb9, 0xcf, 0xc7, 0xca,
	0x35, 0xc2, 0xe5, 0x35, 0xc5, 0x68, 0x35, 0xbe, 0xb6, 0x02, 0x04, 0x4d,
	0x51, 0xc4, 0xf2, 0xbf, 0x04, 0x53, 0x4e, 0xc8, 0xc1, 0xbe, 0x58, 0xc8,
	0xc1, 0xc1, 0x55, 0xc8, 0xc1, 0xbb, 0x5d, 0xc5, 0x64, 0xb6, 0xd9, 0xc7,
	0x75, 0xb8, 0xb0, 0xc3, 0x9e, 0xb5, 0x44, 0xc2, 0x0f, 0xba, 0x29, 0xc2,
	0x0f, 0xb7, 0xa6, 0xc2, 0x0f, 0xbe, 0x58, 0x04, 0x0a, 0x00, 0x01, 0x00,
	0x12, 0x42, 0x19, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x19,
	0x98, 0xc6, 0x19, 0x93, 0x44, 0x19, 0xa2, 0x01, 0x17, 0x84, 0x00, 0x04,
	0x0a, 0x01, 0x01, 0x00, 0x12, 0x42, 0x19, 0x98, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x42, 0x19, 0x98, 0xc7, 0x26, 0x5f, 0x28, 0x96, 0xf9, 0x01,
	0x17, 0x83, 0x00, 0x04, 0x0a, 0x02, 0x01, 0x01, 0x00, 0x0a, 0x03, 0x02,
	0x02, 0x03, 0x00
};

// #pragma mark - PageIconView


class URLInputGroup::PageIconView : public BView {
public:
	PageIconView()
		:
		BView("page icon view", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
		fIcon(NULL),
		fClickPoint(-1, 0),
		fPageIconSet(false)
	{
		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
	}

	~PageIconView()
	{
		delete fIcon;
	}

	virtual void Draw(BRect updateRect)
	{
		BRect bounds(Bounds());
		BRect iconBounds(0, 0, 15, 15);
		iconBounds.OffsetTo(
			floorf((bounds.left + bounds.right
				- (iconBounds.left + iconBounds.right)) / 2 + 0.5f),
			floorf((bounds.top + bounds.bottom
				- (iconBounds.top + iconBounds.bottom)) / 2 + 0.5f));
		DrawBitmap(fIcon, fIcon->Bounds(), iconBounds,
			B_FILTER_BITMAP_BILINEAR);
	}

	virtual BSize MinSize()
	{
		return BSize(18, 18);
	}

	virtual BSize MaxSize()
	{
		return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
	}

	virtual BSize PreferredSize()
	{
		return MinSize();
	}

	void MouseDown(BPoint where)
	{
		int32 buttons;
		if (Window()->CurrentMessage()->FindInt32("buttons", &buttons) == B_OK) {
			if ((buttons & B_PRIMARY_MOUSE_BUTTON) != 0) {
				// Memorize click point for dragging
				fClickPoint = where;
			}
		}
		return;
	}

	void MouseUp(BPoint where)
	{
		fClickPoint.x = -1;
	}

	virtual void MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
	{
		if (dragMessage != NULL)
			return;

		if (fClickPoint.x >= 0
			&& (fabs(where.x - fClickPoint.x) > 4 || fabs(where.y - fClickPoint.y) > 4)) {
			// Start dragging
			BPoint offset = fClickPoint - Frame().LeftTop();

			const char* url = static_cast<URLInputGroup*>(Parent())->Text();
			const char* title =
				static_cast<BWebWindow*>(Window())->CurrentWebView()->MainFrameTitle();

			// Validate the file name to be set for the clipping if user drags to Tracker.
			BString fileName(title);
			if (fileName.Length() == 0) {
				fileName = url;
				int32 leafPos = fileName.FindLast('/');
				if (leafPos >= 0)
					fileName.Remove(0, leafPos + 1);
			}
			fileName.ReplaceAll('/', '-');
			fileName.Truncate(B_FILE_NAME_LENGTH - 1);

			BBitmap miniIcon(BRect(0, 0, 15, 15), B_BITMAP_NO_SERVER_LINK,
				B_CMAP8);
			miniIcon.ImportBits(fIcon);

			// Large icon now provided via PageUserData::PageIconLarge().
			const BBitmap* largeIcon = NULL;
			PageUserData* userData = static_cast<PageUserData*>(
				static_cast<BrowserWindow*>(Window())->CurrentWebView()->GetUserData());
			if (userData != NULL)
				largeIcon = userData->PageIconLarge();

			BMessage drag(B_SIMPLE_DATA);
			drag.AddInt32("be:actions", B_COPY_TARGET);
			drag.AddString("be:clip_name", fileName.String());
			drag.AddString("be:filetypes", "application/x-vnd.Be-bookmark");
			// Support the "Passing Data via File" protocol
			drag.AddString("be:types", B_FILE_MIME_TYPE);
			BMessage data(B_SIMPLE_DATA);
			data.AddString("url", url);
			data.AddString("title", title);
				// The title may differ from the validated filename
			if (fPageIconSet == true) {
				// Don't bother sending the placeholder web icon, if that is all we have.
				data.AddData("miniIcon", B_COLOR_8_BIT_TYPE, &miniIcon, sizeof(miniIcon));
				if (largeIcon != NULL) {
					data.AddData("largeIcon", B_RGBA32_TYPE, largeIcon->Bits(),
						largeIcon->BitsLength());
				}
			}
			drag.AddMessage("be:originator-data", &data);

			BBitmap* iconClone = new BBitmap(fIcon);
			DragMessage(&drag, iconClone, B_OP_ALPHA, offset);
			delete iconClone;
		}
		return;
	}

	void SetIcon(const BBitmap* icon)
	{
		delete fIcon;
		if (icon) {
			fIcon = new BBitmap(icon);
			fPageIconSet = true;
		} else {
			fIcon = new BBitmap(BRect(0, 0, 15, 15), B_RGB32);
			BIconUtils::GetVectorIcon(kPlaceholderIcon,
				sizeof(kPlaceholderIcon), fIcon);
			fPageIconSet = false;
		}
		Invalidate();
	}

private:
	BBitmap* fIcon;
	BPoint fClickPoint;
	bool fPageIconSet;
};


// #pragma mark - URLInputGroup


URLInputGroup::URLInputGroup(BMessage* goMessage)
	:
	BGroupView(B_HORIZONTAL, 0.0),
	fWindowActive(false),
	fURLLocked(false)
{
	GroupLayout()->SetInsets(2, 2, 2, 2);

	fIconView = new PageIconView();
	GroupLayout()->AddView(fIconView, 0.0f);

	// Permissions Button (Shield)
	// We use SHOW_PERMISSIONS_WINDOW message defined in BrowserWindow.h
	// URLInputGroup.cpp already includes BrowserWindow.h
	fPermissionsButton = new BButton(NULL, NULL, new BMessage(SHOW_PERMISSIONS_WINDOW));

	// Create Shield Icon
	BBitmap* shieldIcon = new BBitmap(BRect(0, 0, 15, 15), B_RGBA32, true);
	if (shieldIcon->InitCheck() == B_OK) {
		memset(shieldIcon->Bits(), 0, shieldIcon->BitsLength());
		BView* drawer = new BView(shieldIcon->Bounds(), "drawer", B_FOLLOW_NONE, 0);
		shieldIcon->AddChild(drawer);
		if (shieldIcon->Lock()) {
			drawer->SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
			BPoint points[5];
			points[0] = BPoint(3, 2);
			points[1] = BPoint(12, 2);
			points[2] = BPoint(12, 8);
			points[3] = BPoint(7.5, 14);
			points[4] = BPoint(3, 8);
			drawer->FillPolygon(points, 5);
			drawer->Sync();
			shieldIcon->Unlock();
		}
		shieldIcon->RemoveChild(drawer);
		delete drawer;
		fPermissionsButton->SetIcon(shieldIcon);
		delete shieldIcon;
	} else {
		fPermissionsButton->SetLabel("Site");
		delete shieldIcon;
	}

	fPermissionsButton->SetToolTip("Site Permissions");
	fPermissionsButton->SetExplicitMinSize(BSize(20, 20));
	GroupLayout()->AddView(fPermissionsButton, 0.0f);

	// Search Button (Magnifying Glass)
	fSearchButton = new BButton(NULL, NULL, new BMessage(kMsgSearchButtonClick));

	// Create Magnifying Glass Icon
	BBitmap* searchIcon = new BBitmap(BRect(0, 0, 15, 15), B_RGBA32, true);
	if (searchIcon->InitCheck() == B_OK) {
		memset(searchIcon->Bits(), 0, searchIcon->BitsLength());
		BView* drawer = new BView(searchIcon->Bounds(), "drawer", B_FOLLOW_NONE, 0);
		searchIcon->AddChild(drawer);
		if (searchIcon->Lock()) {
			drawer->SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
			drawer->SetPenSize(1.5f);
			drawer->StrokeEllipse(BRect(3, 3, 10, 10));
			drawer->StrokeLine(BPoint(9, 9), BPoint(13, 13));
			drawer->Sync();
			searchIcon->Unlock();
		}
		searchIcon->RemoveChild(drawer);
		delete drawer;
		fSearchButton->SetIcon(searchIcon);
		delete searchIcon;
	} else {
		fSearchButton->SetLabel("Q");
		delete searchIcon;
	}
	fSearchButton->SetToolTip(B_TRANSLATE("Search Engine"));
	fSearchButton->SetExplicitMinSize(BSize(20, 20));
	GroupLayout()->AddView(fSearchButton, 0.0f);

	fTextView = new URLTextView(this);
	AddChild(fTextView);

	AddChild(new BSeparatorView(B_VERTICAL, B_PLAIN_BORDER));

	// Use standard BButton and set the icon from resources
	fGoButton = new BButton(NULL, NULL, goMessage);

	BResources* resources = BApplication::AppResources();
	if (resources != NULL) {
		size_t size;
		const void* data = resources->LoadResource(B_ARCHIVED_OBJECT, "kActionGo", &size);
		if (data != NULL) {
			BMessage archive;
			if (archive.Unflatten((const char*)data) == B_OK) {
				BBitmap* bitmap = new BBitmap(&archive);
				if (bitmap->InitCheck() == B_OK) {
					fGoButton->SetIcon(bitmap);
				}
				delete bitmap;
			}
		}
	}

	GroupLayout()->AddView(fGoButton, 0.0f);

	SetFlags(Flags() | B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);
	SetLowColor(ViewColor());
	SetViewColor(fTextView->ViewColor());

	SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
		B_ALIGN_VERTICAL_CENTER));

}


URLInputGroup::~URLInputGroup()
{
}


void
URLInputGroup::AttachedToWindow()
{
	BGroupView::AttachedToWindow();
	fWindowActive = Window()->IsActive();
	fSearchButton->SetTarget(this);
}

void
URLInputGroup::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSearchButtonClick:
		{
			BPopUpMenu* menu = new BPopUpMenu("search engines");
			for (int32 i = 0; kSearchEngines[i].url != NULL; i++) {
				BMessage* msg = new BMessage(SET_SEARCH_ENGINE);
				msg->AddString("url", kSearchEngines[i].url);
				BMenuItem* item = new BMenuItem(kSearchEngines[i].name, msg);
				menu->AddItem(item);
			}
			menu->SetTargetForItems(Window());
			BRect buttonFrame = fSearchButton->Frame();
			BPoint where = ConvertToScreen(buttonFrame.LeftBottom());
			menu->Go(where, true, true, true);
			break;
		}
		default:
			BGroupView::MessageReceived(message);
	}
}


void
URLInputGroup::WindowActivated(bool active)
{
	BGroupView::WindowActivated(active);
	if (fWindowActive != active) {
		fWindowActive = active;
		Invalidate();
	}
}


void
URLInputGroup::Draw(BRect updateRect)
{
	BRect bounds(Bounds());
	rgb_color base(LowColor());
	uint32 flags = 0;
	if (fWindowActive && fTextView->IsFocus())
		flags |= BControlLook::B_FOCUSED;
	be_control_look->DrawTextControlBorder(this, bounds, updateRect, base,
		flags);
}


void
URLInputGroup::MakeFocus(bool focus)
{
	// Forward this to the text view, we never accept focus ourselves.
	fTextView->MakeFocus(focus);
}


BTextView*
URLInputGroup::TextView() const
{
	return fTextView;
}


void
URLInputGroup::SetText(const char* text)
{
	// Ignore setting the text, if the input is locked.
	if (fURLLocked)
		return;

	if (!text || !Text() || strcmp(Text(), text) != 0) {
		fTextView->SetUpdateAutoCompleterChoices(false);
		fTextView->SetText(text);
		fTextView->SetUpdateAutoCompleterChoices(true);
	}
}


const char*
URLInputGroup::Text() const
{
	return fTextView->Text();
}


BButton*
URLInputGroup::GoButton() const
{
	return fGoButton;
}


void
URLInputGroup::SetPageIcon(const BBitmap* icon)
{
	fIconView->SetIcon(icon);
}


bool
URLInputGroup::IsURLInputLocked() const
{
	return fURLLocked;
}


void
URLInputGroup::LockURLInput(bool lock)
{
	fURLLocked = lock;
}


void
URLInputGroup::SetPrivateMode(bool privateMode)
{
	rgb_color color = ui_color(B_DOCUMENT_BACKGROUND_COLOR);
	if (privateMode)
		color = tint_color(color, B_DARKEN_1_TINT);

	fTextView->SetViewColor(color);
	fTextView->SetLowColor(color);
	fTextView->Invalidate();

	SetLowColor(color);
	Invalidate();
}
