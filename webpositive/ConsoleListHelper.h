/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef CONSOLE_LIST_HELPER_H
#define CONSOLE_LIST_HELPER_H

#include <deque>
#include <functional>
#include <String.h>

#include "ConsoleMessage.h"

class IConsoleList {
public:
	virtual ~IConsoleList() {}
	virtual int32 CountItems() const = 0;
	virtual void RemoveItem(int32 index) = 0;
	virtual void AddItem(const char* text) = 0;
	virtual void SetItemText(int32 index, const char* text) = 0;
	virtual const char* GetItemText(int32 index) const = 0;
};

// Returns the formatted message text
typedef std::function<BString(const ConsoleMessage&)> MessageFormatter;
// Returns the formatted repeat text
typedef std::function<BString(int32 count)> RepeatFormatter;

void UpdateConsoleMessageList(
	IConsoleList& list,
	const std::deque<ConsoleMessage>& messages,
	bool errorsOnly,
	BString& previousText,
	int32& repeatCounter,
	MessageFormatter formatMessage,
	RepeatFormatter formatRepeat);

#endif // CONSOLE_LIST_HELPER_H
