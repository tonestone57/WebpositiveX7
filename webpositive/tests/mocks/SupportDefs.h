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
typedef uint32 type_code;

const status_t B_OK = 0;
const status_t B_ERROR = -1;
const status_t B_NO_MEMORY = -2;
const status_t B_IO_ERROR = -3;
const status_t B_BAD_VALUE = -4; // Added B_BAD_VALUE
const status_t B_ENTRY_NOT_FOUND = -5;
const status_t B_ALREADY_RUNNING = -6;
const status_t B_NAME_NOT_FOUND = -7;
const uint32 B_NO_REPLY = 0;
const type_code B_COLOR_8_BIT_TYPE = 1;
const type_code B_STRING_TYPE = 'CSTR';
const type_code B_RAW_TYPE = 'RAWT';
const type_code B_ANY_TYPE = 'ANYT';
#ifndef B_RGBA32_TYPE // Defined in WebConstants.h usually, but here for mocks
#define B_RGBA32_TYPE 0x52474241
#endif
#define B_FILE_NAME_LENGTH 256
#define B_ATTR_NAME_LENGTH 256
#endif
