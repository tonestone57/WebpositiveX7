#ifndef _MOCK_SUPPORT_DEFS_H
#define _MOCK_SUPPORT_DEFS_H
#include <stdint.h>
#include <sys/types.h>

typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;
typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;

typedef int32 status_t;
typedef int64 off_t;
typedef int64 bigtime_t;

const status_t B_OK = 0;
const status_t B_ERROR = -1;
const status_t B_NO_MEMORY = -2;
const status_t B_IO_ERROR = -3;
const status_t B_BAD_VALUE = -4; // Added B_BAD_VALUE
#endif
