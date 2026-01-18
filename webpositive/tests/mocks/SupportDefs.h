#ifndef SUPPORT_DEFS_H
#define SUPPORT_DEFS_H
#include <stdint.h>
#include <inttypes.h>
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;
typedef int32_t status_t;
typedef int64_t bigtime_t;
typedef uint32_t type_code;
#define B_OK 0
#define B_ERROR -1
#define B_BAD_VALUE -2

#ifndef B_PRId64
#define B_PRId64 PRId64
#endif

#endif
