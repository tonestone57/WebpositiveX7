/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef PAGE_SOURCE_SAVER_H
#define PAGE_SOURCE_SAVER_H

#include <SupportDefs.h>

class BMessage;

class PageSourceSaver {
public:
	static void HandlePageSourceResult(const BMessage* message);

private:
	static status_t _PageSourceThread(void* data);
};

#endif // PAGE_SOURCE_SAVER_H
