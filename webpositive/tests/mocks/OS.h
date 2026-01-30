#ifndef _OS_H
#define _OS_H
#include "SupportDefs.h"
typedef int32_t thread_id;
typedef int64_t bigtime_t;
status_t resume_thread(thread_id);
status_t kill_thread(thread_id);
thread_id spawn_thread(status_t (*func)(void*), const char* name, int32 priority, void* data);
#define B_NORMAL_PRIORITY 10
#define B_LOW_PRIORITY 5
int32_t atomic_add(int32_t* value, int32_t addvalue);
int32_t atomic_get(int32_t* value);
void snooze(bigtime_t microseconds);
#endif
