/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef MOCK_OS_H
#define MOCK_OS_H

#include <SupportDefs.h>
#include <unistd.h>
#include <time.h>

typedef int64 bigtime_t;

inline bigtime_t system_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (bigtime_t)(ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
}

inline status_t snooze(bigtime_t microseconds) {
    usleep(microseconds);
    return B_OK;
}

enum {
	B_LOW_PRIORITY = 10,
	B_NORMAL_PRIORITY = 20
};

#endif
