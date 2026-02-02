/*
 * Copyright 2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "BaseURL.h"


BString
baseURL(const BString& string)
{
	BString result;
	baseURL(string, result);
	return result;
}


void
baseURL(const BString& string, BString& result)
{
	int32 protoPos = string.FindFirst("://");
	if (protoPos < 0) {
		result = string;
		return;
	}

	int32 baseURLStart = protoPos + 3;
	int32 baseURLEnd = string.FindFirst("/", baseURLStart + 1);
	result.SetTo(string.String() + baseURLStart, baseURLEnd - baseURLStart);
}
