/*
 * Copyright 2014-2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Zhuowei Zhang
 *		Humdinger
 */
#include "ConsoleWindow.h"

#include <Catalog.h>
#include <Clipboard.h>
#include <Message.h>
#include <Button.h>
#include <CheckBox.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>
#include <StringFormat.h>
#include <TextControl.h>
#include <ListView.h>
#include <ScrollView.h>

#include "BrowserWindow.h"
#include "BrowserApp.h"
#include "WebViewConstants.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Console Window"


enum {
	EVAL_CONSOLE_WINDOW_COMMAND = 'ecwc',
	CLEAR_CONSOLE_MESSAGES = 'ccms',
	FILTER_ERRORS_ONLY = 'feon'
};


ConsoleWindow::ConsoleWindow(BRect frame)
	:
	BWindow(frame, B_TRANSLATE("Script console"), B_TITLED_WINDOW,
		B_NORMAL_WINDOW_FEEL, B_AUTO_UPDATE_SIZE_LIMITS
			| B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE),
	fQuitting(false),
	fPreviousText(""),
	fRepeatCounter(0)
{
	SetLayout(new BGroupLayout(B_VERTICAL, 0.0));

	fMessagesListView = new BListView("Console messages",
		B_MULTIPLE_SELECTION_LIST);

	fClearMessagesButton = new BButton(B_TRANSLATE("Clear"),
		new BMessage(CLEAR_CONSOLE_MESSAGES));
	fCopyMessagesButton = new BButton(B_TRANSLATE("Copy"),
		new BMessage(B_COPY));
	fErrorsOnlyCheckBox = new BCheckBox("Errors only", new BMessage(FILTER_ERRORS_ONLY));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0.0)
		.Add(new BScrollView("Console messages scroll",
			fMessagesListView, 0, true, true))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.Add(fErrorsOnlyCheckBox)
			.AddGlue()
			.Add(fClearMessagesButton)
			.Add(fCopyMessagesButton)
			.AddGlue()
			.SetInsets(0, B_USE_SMALL_SPACING, 0, 0))
		.SetInsets(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING,
			B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
	);
	if (!frame.IsValid())
		CenterOnScreen();
}


ConsoleWindow::~ConsoleWindow()
{
	int32 count = fMessagesListView->CountItems();
	for (int32 i = count - 1; i >= 0; i--)
		delete fMessagesListView->RemoveItem(i);
}


void
ConsoleWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case ADD_CONSOLE_MESSAGE:
		{
			BString source = message->FindString("source");
			int32 lineNumber = message->FindInt32("line");
			int32 columnNumber = message->FindInt32("column");
			BString text = message->FindString("string");
			// Crude filtering for errors only
			if (fErrorsOnlyCheckBox->Value() == B_CONTROL_ON) {
				if (text.FindFirst("Error") == B_ERROR && text.FindFirst("error") == B_ERROR && text.FindFirst("Exception") == B_ERROR) {
					// Likely not an error, skip
					break;
				}
			}

			BString finalText;
			finalText.SetToFormat("%s:%" B_PRIi32 ":%" B_PRIi32 ": %s\n",
				source.String(), lineNumber, columnNumber, text.String());

			if (finalText == fPreviousText) {
				finalText = "";
				static BStringFormat format(B_TRANSLATE("{0, plural,"
					"one{Last line repeated # time.}"
					"other{Last line repeated # times.}}"));
				format.Format(finalText, ++fRepeatCounter);
				// preserve the repeated line
				if (fRepeatCounter > 1) {
					int32 index = fMessagesListView->CountItems() - 1;
					BStringItem* item = (BStringItem*)fMessagesListView->ItemAt(index);
					item->SetText(finalText.String());
					fMessagesListView->InvalidateItem(index);
					break;
				}
			} else {
				fPreviousText = finalText;
				fRepeatCounter = 0;
			}
			fMessagesListView->AddItem(new BStringItem(finalText.String()));
			if (fMessagesListView->CountItems() > 500)
				delete fMessagesListView->RemoveItem(0);
			break;
		}
		case CLEAR_CONSOLE_MESSAGES:
		{
			fPreviousText = "";
			int32 count = fMessagesListView->CountItems();
			for (int32 i = count - 1; i >= 0; i--)
				delete fMessagesListView->RemoveItem(i);
			break;
		}
		case B_COPY:
		{
			_CopyToClipboard();
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
ConsoleWindow::QuitRequested()
{
	if (fQuitting)
		return true;

	if (!IsHidden())
		Hide();
	return false;
}


void
ConsoleWindow::PrepareToQuit()
{
	fQuitting = true;
}


void
ConsoleWindow::_CopyToClipboard()
{
	BString text;
	int32 index;
	if (fMessagesListView->CurrentSelection() == -1) {
		for (int32 i = 0; i < fMessagesListView->CountItems(); i++) {
			BStringItem* item = (BStringItem*)fMessagesListView->ItemAt(i);
			text << item->Text();
		}
	} else {
		for (int32 i = 0; (index = fMessagesListView->CurrentSelection(i)) >= 0; i++) {
			BStringItem* item = (BStringItem*)fMessagesListView->ItemAt(index);
			text << item->Text();
		}
	}

	ssize_t textLen = text.Length();
	if (be_clipboard->Lock()) {
		be_clipboard->Clear();
		BMessage* clip = be_clipboard->Data();
		if (clip != NULL) {
			clip->AddData("text/plain", B_MIME_TYPE, text.String(), textLen);
			be_clipboard->Commit();
		}
		be_clipboard->Unlock();
	}
}
