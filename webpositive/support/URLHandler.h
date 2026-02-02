/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef URL_HANDLER_H
#define URL_HANDLER_H

#include <String.h>

class URLHandler {
public:
	enum Action {
		DO_NOTHING,
		LOAD_URL,
		LAUNCH_APP
	};

	static Action CheckURL(const BString& input, BString& outURL, const BString& searchPageURL);
	static bool IsValidDomainChar(char ch);
	static BString EncodeURIComponent(const BString& search);

private:
	static void _VisitSearchEngine(const BString& search, BString& outURL, const BString& searchPageURL);
};

#endif // URL_HANDLER_H
