/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef FORM_SAFETY_HELPER_H
#define FORM_SAFETY_HELPER_H

#include <SupportDefs.h>
#include <String.h>

#include <memory>

class BMessage;
class BMessageRunner;
class BWebView;
class BrowserWindow;

class FormSafetyHelper {
public:
								FormSafetyHelper(BrowserWindow* window);
								~FormSafetyHelper();

			bool				QuitRequested();
			void				MessageReceived(BMessage* message);
			void				ConsoleMessage(const BString& message);

			bool				IsFormCheckPending() const { return fFormCheckPending; }
			bool				IsForceClose() const { return fForceClose; }

private:
			void				_CheckFormDirtyFinished();

private:
			BrowserWindow*		fWindow;
			bool				fFormCheckPending;
			bool				fForceClose;
			int32				fTabsToCheck;
			int32				fDirtyTabs;
			std::unique_ptr<BMessageRunner>		fFormCheckTimeoutRunner;
};

#endif // FORM_SAFETY_HELPER_H
