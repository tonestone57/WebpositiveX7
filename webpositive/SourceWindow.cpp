/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "SourceWindow.h"

#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <TextView.h>

SourceWindow::SourceWindow(const char* title, const BString& text)
	:
	BWindow(BRect(100, 100, 700, 500), title, B_TITLED_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS)
{
	fTextView = new BTextView("source view");
	fTextView->MakeEditable(false);
	fTextView->SetText(text.String());

	// Basic Monospace font
	BFont font(be_fixed_font);
	fTextView->SetFontAndColor(&font);

	_HighlightSyntax();

	BScrollView* scrollView = new BScrollView("scroll", fTextView, 0, false, true);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(scrollView)
		.SetInsets(0);

	CenterOnScreen();
}

SourceWindow::~SourceWindow()
{
}

void
SourceWindow::_HighlightSyntax()
{
	const char* text = fTextView->Text();
	int32 length = fTextView->TextLength();

	rgb_color tagColor = {0, 0, 180, 255};      // Dark Blue
	rgb_color attrColor = {128, 0, 0, 255};     // Dark Red
	rgb_color valueColor = {0, 100, 0, 255};    // Dark Green
	rgb_color commentColor = {128, 128, 128, 255}; // Gray
	rgb_color normalColor = {0, 0, 0, 255};

	// Reset to normal
	fTextView->SetFontAndColor(0, length, NULL, 0, &normalColor);

	// Very simple state machine for basic HTML highlighting
	enum State {
		TEXT,
		TAG_START,    // <
		TAG_NAME,     // html
		WAIT_ATTR,    // space after tag name
		ATTR_NAME,    // href
		WAIT_VALUE,   // =
		VALUE_Q,      // "value"
		VALUE_SQ,     // 'value'
		COMMENT_START,// <!
		COMMENT,      // --
		COMMENT_END   // -->
	};

	State state = TEXT;
	int32 startPos = 0;

	for (int32 i = 0; i < length; i++) {
		char c = text[i];

		switch (state) {
			case TEXT:
				if (c == '<') {
					startPos = i;
					if (i + 3 < length && text[i+1] == '!' && text[i+2] == '-' && text[i+3] == '-') {
						state = COMMENT;
						i += 3;
					} else {
						state = TAG_START;
					}
					fTextView->SetFontAndColor(i, i+1, NULL, 0, &tagColor);
				}
				break;
			case TAG_START:
				if (c == ' ' || c == '\t' || c == '\n') {
					// < space? invalid but handled
				} else if (c == '>') {
					fTextView->SetFontAndColor(i, i+1, NULL, 0, &tagColor);
					state = TEXT;
				} else if (c == '/') {
					fTextView->SetFontAndColor(i, i+1, NULL, 0, &tagColor);
				} else {
					state = TAG_NAME;
					startPos = i;
				}
				break;
			case TAG_NAME:
				if (c == ' ' || c == '\t' || c == '\n') {
					fTextView->SetFontAndColor(startPos, i, NULL, 0, &tagColor);
					state = WAIT_ATTR;
				} else if (c == '>') {
					fTextView->SetFontAndColor(startPos, i, NULL, 0, &tagColor);
					fTextView->SetFontAndColor(i, i+1, NULL, 0, &tagColor);
					state = TEXT;
				} else if (c == '/') {
					fTextView->SetFontAndColor(startPos, i, NULL, 0, &tagColor);
					fTextView->SetFontAndColor(i, i+1, NULL, 0, &tagColor);
				}
				break;
			case WAIT_ATTR:
				if (c == '>') {
					fTextView->SetFontAndColor(i, i+1, NULL, 0, &tagColor);
					state = TEXT;
				} else if (c != ' ' && c != '\t' && c != '\n' && c != '/') {
					state = ATTR_NAME;
					startPos = i;
				} else if (c == '/') {
					fTextView->SetFontAndColor(i, i+1, NULL, 0, &tagColor);
				}
				break;
			case ATTR_NAME:
				if (c == '=') {
					fTextView->SetFontAndColor(startPos, i, NULL, 0, &attrColor);
					state = WAIT_VALUE;
				} else if (c == ' ' || c == '\t' || c == '\n' || c == '>') {
					fTextView->SetFontAndColor(startPos, i, NULL, 0, &attrColor);
					if (c == '>') {
						fTextView->SetFontAndColor(i, i+1, NULL, 0, &tagColor);
						state = TEXT;
					} else {
						state = WAIT_ATTR;
					}
				}
				break;
			case WAIT_VALUE:
				if (c == '"') {
					state = VALUE_Q;
					startPos = i;
				} else if (c == '\'') {
					state = VALUE_SQ;
					startPos = i;
				} else if (c != ' ' && c != '\t' && c != '\n') {
					// Unquoted value (not supported strictly here, treated as attr or text)
					state = WAIT_ATTR;
				}
				break;
			case VALUE_Q:
				if (c == '"') {
					fTextView->SetFontAndColor(startPos, i+1, NULL, 0, &valueColor);
					state = WAIT_ATTR;
				}
				break;
			case VALUE_SQ:
				if (c == '\'') {
					fTextView->SetFontAndColor(startPos, i+1, NULL, 0, &valueColor);
					state = WAIT_ATTR;
				}
				break;
			case COMMENT:
				if (c == '-' && i + 2 < length && text[i+1] == '-' && text[i+2] == '>') {
					fTextView->SetFontAndColor(startPos, i+3, NULL, 0, &commentColor);
					i += 2;
					state = TEXT;
				}
				break;
			default:
				break;
		}
	}
}
