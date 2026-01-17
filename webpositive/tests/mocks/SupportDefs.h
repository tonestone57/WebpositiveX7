/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SUPPORT_DEFS_H
#define _SUPPORT_DEFS_H

#include <stdint.h>
#include <sys/types.h>

typedef int32_t status_t;
typedef int64_t bigtime_t;
typedef uint32_t type_code;
typedef uint32_t uint32;
typedef int32_t int32;
typedef uint64_t uint64;
typedef int64_t int64;

#define B_OK 0
#define B_ERROR -1
#define B_NAME_NOT_FOUND -2
#define B_ENTRY_NOT_FOUND -3
#define B_BAD_VALUE -4
#define B_NO_MEMORY -5
#define B_TIMED_OUT -6
#define B_ALREADY_RUNNING -7

// File open modes
#define B_READ_ONLY 0x00
#define B_WRITE_ONLY 0x01
#define B_READ_WRITE 0x02
#define B_FAIL_IF_EXISTS 0x04
#define B_CREATE_FILE 0x08
#define B_ERASE_FILE 0x10
#define B_OPEN_AT_END 0x20

#endif // _SUPPORT_DEFS_H
