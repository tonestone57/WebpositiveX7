#ifndef _SUPPORT_DEFS_H
#define _SUPPORT_DEFS_H

#include <stdint.h>
#include <sys/types.h>

#ifndef _HAIKU_SUPPORT_DEFS_H
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;
typedef int32_t status_t;
typedef int64_t bigtime_t;
typedef uint32_t type_code;

const int32 B_OK = 0;
const int32 B_ERROR = -1;
const int32 B_BAD_VALUE = -2;
#endif

#endif
