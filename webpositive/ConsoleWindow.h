/*
 * Copyright 2014-2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Zhuowei Zhang
 *		Humdinger
 */
#ifndef CONSOLE_WINDOW_H
#define CONSOLE_WINDOW_H


#include <String.h>
#include <Window.h>

#include <deque>

class BListView;
class BButton;
class BCheckBox;


class ConsoleWindow : public BWindow {
public:
								ConsoleWindow(BRect frame);
	virtual						~ConsoleWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

			void				PrepareToQuit();

private:
			void				_CopyToClipboard();
			void				_UpdateMessageList();

private:
			struct ConsoleMessage {
				BString text;
				BString source;
				int32 line;
				int32 column;
				bool isError;
			};

			bool				fQuitting;
			BListView*			fMessagesListView;
			BButton* 			fClearMessagesButton;
			BButton* 			fCopyMessagesButton;
			BCheckBox*			fErrorsOnlyCheckBox;
			std::deque<ConsoleMessage> fAllMessages;
};


#endif // CONSOLE_WINDOW_H
