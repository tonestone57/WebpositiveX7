/*
 * Copyright 2014-2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef CONSOLE_MESSAGE_H
#define CONSOLE_MESSAGE_H

#include <String.h>

struct ConsoleMessage {
	BString text;
	BString source;
	int32 line;
	int32 column;
	bool isError;
};

#endif // CONSOLE_MESSAGE_H
