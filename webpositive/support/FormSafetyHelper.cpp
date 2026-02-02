/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "FormSafetyHelper.h"
#include "BrowserWindow.h"
#include "TabManager.h"
#include "WebView.h"
#include "WebPage.h"

#include <Alert.h>
#include <Catalog.h>
#include <Invoker.h>
#include <MessageRunner.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Form Safety Helper"

FormSafetyHelper::FormSafetyHelper(BrowserWindow* window)
	:
	fWindow(window),
	fFormCheckPending(false),
	fForceClose(false),
	fTabsToCheck(0),
	fDirtyTabs(0)
{
}


FormSafetyHelper::~FormSafetyHelper()
{
}


bool
FormSafetyHelper::QuitRequested()
{
	if (fForceClose) {
		// Cleanup happens in window
		return true;
	}

	if (fFormCheckPending)
		return false;

	fFormCheckPending = true;
	fTabsToCheck = 0;
	fDirtyTabs = 0;

	BString checkScript(
		"(function(){"
		"    var dirty = false;"
		"    try {"
		"        var forms = document.forms;"
		"        for (var i = 0; i < forms.length; i++) {"
		"            var form = forms[i];"
		"            for (var j = 0; j < form.elements.length; j++) {"
		"                var el = form.elements[j];"
		"                if (el.disabled || el.readOnly) continue;"
		"                var type = el.type;"
		"                if (type == 'checkbox' || type == 'radio') {"
		"                    if (el.checked != el.defaultChecked) { dirty = true; break; }"
		"                } else if (type == 'select-one' || type == 'select-multiple') {"
		"                     for (var k = 0; k < el.options.length; k++) {"
		"                         if (el.options[k].selected != el.options[k].defaultSelected) { dirty = true; break; }"
		"                     }"
		"                } else if (type == 'text' || type == 'textarea' || type == 'password' || type == 'search' || type == 'email' || type == 'url' || type == 'tel' || type == 'number') {"
		"                    if (el.value != el.defaultValue) { dirty = true; break; }"
		"                }"
		"            }"
		"            if (dirty) break;"
		"        }"
		"    } catch(e) {}"
		"    console.log('WebPositive:FormDirty:' + (dirty ? 'true' : 'false'));"
		"})();"
	);

	TabManager* tabManager = fWindow->GetTabManager();
	for (int32 i = 0; i < tabManager->CountTabs(); i++) {
		BWebView* view = dynamic_cast<BWebView*>(tabManager->ViewForTab(i));
		if (view == NULL)
			continue;

		BString url = view->MainFrameURL();
		if (url.Length() == 0 || url == "about:blank")
			continue;

		fTabsToCheck++;
		view->WebPage()->EvaluateJavaScript(checkScript);
	}

	if (fTabsToCheck == 0) {
		fFormCheckPending = false;
		fForceClose = true;
		fWindow->PostMessage(B_QUIT_REQUESTED);
		return false;
	}

	// Wait up to 1 second for results
	BMessage msg(CHECK_FORM_DIRTY_TIMEOUT);
	fFormCheckTimeoutRunner.reset(new BMessageRunner(BMessenger(fWindow), &msg, 1000000, 1));

	return false;
}


void
FormSafetyHelper::MessageReceived(BMessage* message)
{
	if (message->what == CHECK_FORM_DIRTY_TIMEOUT) {
		if (fFormCheckPending) {
			_CheckFormDirtyFinished();
		}
	}
}


void
FormSafetyHelper::ConsoleMessage(const BString& message)
{
	if (message.Compare("WebPositive:FormDirty:", 22) == 0) {
		if (fFormCheckPending) {
			if (strncmp(message.String() + 22, "true", 4) == 0)
				fDirtyTabs++;

			if (fTabsToCheck > 0)
				fTabsToCheck--;

			if (fTabsToCheck <= 0) {
				_CheckFormDirtyFinished();
			}
		}
	}
}


void
FormSafetyHelper::_CheckFormDirtyFinished()
{
	fFormCheckTimeoutRunner.reset();
	fFormCheckPending = false;

	if (fDirtyTabs > 0) {
		BAlert* alert = new BAlert(B_TRANSLATE("Unsaved data"),
			B_TRANSLATE("One or more tabs have unsaved form data. Do you want to close the window anyway?"),
			B_TRANSLATE("Cancel"), B_TRANSLATE("Close"));
		alert->SetShortcut(0, B_ESCAPE);
		alert->Go(new BInvoker(new BMessage(FORM_SAFETY_ALERT_CONFIRMED), fWindow));
	} else {
		// No dirty tabs, proceed to close
		fForceClose = true;
		fWindow->PostMessage(B_QUIT_REQUESTED);
	}
}


void
FormSafetyHelper::AlertConfirmed(BMessage* message)
{
	int32 which;
	if (message->FindInt32("which", &which) == B_OK && which == 1) { // Close
		fForceClose = true;
		fWindow->PostMessage(B_QUIT_REQUESTED);
	}
}
