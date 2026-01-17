/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SOURCE_WINDOW_H
#define SOURCE_WINDOW_H

#include <Window.h>
#include <String.h>

class BTextView;

class SourceWindow : public BWindow {
public:
	SourceWindow(const char* title, const BString& text);
	virtual ~SourceWindow();

private:
	void _HighlightSyntax();

	BTextView* fTextView;
};

#endif // SOURCE_WINDOW_H
