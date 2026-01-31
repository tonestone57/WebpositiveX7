/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef WEB_CONSTANTS_H
#define WEB_CONSTANTS_H

#include <SupportDefs.h>

// 'RGBA' in ASCII (R=0x52, G=0x47, B=0x42, A=0x41)
// Defined as explicit integer to ensure portability across architectures.
#ifndef B_RGBA32_TYPE
#define B_RGBA32_TYPE 0x52474241
#endif

#endif // WEB_CONSTANTS_H
