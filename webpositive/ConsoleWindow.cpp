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
	fErrorsOnlyCheckBox->SetTarget(this);

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
			ConsoleMessage msg;
			msg.source = message->FindString("source");
			msg.line = message->FindInt32("line");
			msg.column = message->FindInt32("column");
			msg.text = message->FindString("string");
			msg.isError = (msg.text.FindFirst("Error") != B_ERROR
				|| msg.text.FindFirst("error") != B_ERROR
				|| msg.text.FindFirst("Exception") != B_ERROR);

			fAllMessages.push_back(msg);
			if (fAllMessages.size() > 500)
				fAllMessages.pop_front();

			if (!IsHidden()) {
				_AppendMessage(msg);
				// Periodically re-sync list to prevent unbounded growth from collapsed items
				// or drift from deque popping
				if (fMessagesListView->CountItems() > 2000)
					_UpdateMessageList();
				else if (fMessagesListView->CountItems() > 0)
					fMessagesListView->ScrollTo(fMessagesListView->CountItems() - 1);
			}
			break;
		}
		case CLEAR_CONSOLE_MESSAGES:
		{
			fAllMessages.clear();
			_UpdateMessageList();
			break;
		}
		case FILTER_ERRORS_ONLY:
		{
			_UpdateMessageList();
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
ConsoleWindow::Show()
{
	if (IsHidden())
		_UpdateMessageList();
	BWindow::Show();
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


void
ConsoleWindow::_UpdateMessageList()
{
	// Clear existing items
	int32 count = fMessagesListView->CountItems();
	for (int32 i = count - 1; i >= 0; i--)
		delete fMessagesListView->RemoveItem(i);

	fPreviousText = "";
	fRepeatCounter = 0;

	for (std::deque<ConsoleMessage>::iterator it = fAllMessages.begin();
			it != fAllMessages.end(); ++it) {
		_AppendMessage(*it);
	}

	if (fMessagesListView->CountItems() > 0)
		fMessagesListView->ScrollTo(fMessagesListView->CountItems() - 1);
}


void
ConsoleWindow::_AppendMessage(const ConsoleMessage& message)
{
	bool errorsOnly = fErrorsOnlyCheckBox->Value() == B_CONTROL_ON;

	if (errorsOnly && !message.isError)
		return;

	static BStringFormat repeatFormat(B_TRANSLATE("{0, plural,"
		"one{Last line repeated # time.}"
		"other{Last line repeated # times.}}"));

	BString finalText;
	finalText.SetToFormat("%s:%" B_PRIi32 ":%" B_PRIi32 ": %s\n",
		message.source.String(), message.line, message.column, message.text.String());

	if (finalText == fPreviousText) {
		fRepeatCounter++;
		BString repeatText;
		repeatFormat.Format(repeatText, fRepeatCounter);
		repeatText += "\n";

		int32 lastIdx = fMessagesListView->CountItems() - 1;
		if (lastIdx >= 0) {
			if (fRepeatCounter > 1) {
				// Update existing repeat message
				BStringItem* item = (BStringItem*)fMessagesListView->ItemAt(lastIdx);
				item->SetText(repeatText.String());
			} else {
				// Add new repeat message
				fMessagesListView->AddItem(new BStringItem(repeatText.String()));
			}
		}
	} else {
		fPreviousText = finalText;
		fRepeatCounter = 0;
		fMessagesListView->AddItem(new BStringItem(finalText.String()));
	}
}
