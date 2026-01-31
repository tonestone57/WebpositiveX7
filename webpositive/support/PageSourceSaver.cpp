/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "PageSourceSaver.h"

#include <Alert.h>
#include <Catalog.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <OS.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "SafeStrerror.h"
#include "SourceWindow.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Page Source Saver"

void
PageSourceSaver::HandlePageSourceResult(const BMessage* message)
{
	BString source;
	if (message->FindString("source", &source) == B_OK) {
		BString url;
		message->FindString("url", &url);
		BString title = "Source: ";
		title << url;
		SourceWindow* window = new SourceWindow(title.String(), source);
		window->Show();
		return;
	}

	BMessage* copy = new BMessage(*message);
	thread_id thread = spawn_thread(_PageSourceThread, "Page Source Saver",
		B_NORMAL_PRIORITY, copy);
	if (thread >= 0) {
		if (resume_thread(thread) != B_OK) {
			kill_thread(thread);
			delete copy;
		}
	} else
		delete copy;
}

status_t
PageSourceSaver::_PageSourceThread(void* data)
{
	BMessage* message = static_cast<BMessage*>(data);
	BPath pathToPageSource;

	BString url;
	status_t ret = message->FindString("url", &url);
	if (ret == B_OK && url.FindFirst("file://") == 0) {
		// Local file
		url.Remove(0, strlen("file://"));
		pathToPageSource.SetTo(url.String());
	} else {
		// Something else, store it.
		BString source;
		ret = message->FindString("source", &source);

		if (ret == B_OK)
			ret = find_directory(B_SYSTEM_TEMP_DIRECTORY, &pathToPageSource);

		BString extension = ".html";
		const char* mimeType = "text/html";

		BString urlForExtension(url);
		int32 queryPos = urlForExtension.FindFirst('?');
		if (queryPos != -1)
			urlForExtension.Truncate(queryPos);
		int32 fragmentPos = urlForExtension.FindFirst('#');
		if (fragmentPos != -1)
			urlForExtension.Truncate(fragmentPos);

		int32 dotPos = urlForExtension.FindLast('.');
		int32 slashPos = urlForExtension.FindLast('/');
		if (dotPos > slashPos) {
			BString ext;
			urlForExtension.CopyInto(ext, dotPos + 1, urlForExtension.Length() - dotPos - 1);
			if (ext.Length() > 0 && ext.Length() <= 5) {
				bool valid = true;
				for (int32 i = 0; i < ext.Length(); i++) {
					if (!isalnum(ext[i])) {
						valid = false;
						break;
					}
				}

				if (valid) {
					extension = "";
					extension << "." << ext;
					if (ext.ICompare("svg") == 0)
						mimeType = "image/svg+xml";
					else if (ext.ICompare("xml") == 0)
						mimeType = "text/xml";
					else if (ext.ICompare("txt") == 0)
						mimeType = "text/plain";
				}
			}
		}

		BString tmpFileName("PageSource_");
		tmpFileName << system_time() << extension;
		if (ret == B_OK)
			ret = pathToPageSource.Append(tmpFileName.String());

		BFile pageSourceFile(pathToPageSource.Path(),
			B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
		if (ret == B_OK)
			ret = pageSourceFile.InitCheck();

		if (ret == B_OK) {
			ssize_t written = pageSourceFile.Write(source.String(),
				source.Length());
			if (written != source.Length())
				ret = (written < 0) ? (status_t)written : B_IO_ERROR;
		}

		if (ret == B_OK) {
			size_t size = strlen(mimeType);
			pageSourceFile.WriteAttr("BEOS:TYPE", B_STRING_TYPE, 0, mimeType, size);
				// If it fails we don't care.
		}
	}

	entry_ref ref;
	if (ret == B_OK)
		ret = get_ref_for_path(pathToPageSource.Path(), &ref);

	if (ret == B_OK) {
		BMessage refsMessage(B_REFS_RECEIVED);
		ret = refsMessage.AddRef("refs", &ref);
		if (ret == B_OK) {
			ret = be_roster->Launch("text/x-source-code", &refsMessage);
			if (ret == B_ALREADY_RUNNING)
				ret = B_OK;
		}
	}

	if (ret != B_OK) {
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "Failed to show the "
			"page source: %s\n", SafeStrerror(ret).String());
		BAlert* alert = new BAlert(B_TRANSLATE("Page source error"), buffer,
			B_TRANSLATE("OK"));
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go(NULL);
	}

	delete message;
	return B_OK;
}
