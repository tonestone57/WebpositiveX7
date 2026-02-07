#ifndef _MOCK_OS_H
#define _MOCK_OS_H

#include "SupportDefs.h"

typedef int32 thread_id;

status_t spawn_thread(status_t (*func)(void*), const char* name, int32 priority, void* data);
status_t resume_thread(thread_id thread);
status_t kill_thread(thread_id thread);
int32_t atomic_add(int32_t* value, int32_t addvalue);
int32_t atomic_get(int32_t* value);
void snooze(bigtime_t microseconds);

enum {
    B_NORMAL_PRIORITY = 10,
    B_LOW_PRIORITY = 5
};

#endif
