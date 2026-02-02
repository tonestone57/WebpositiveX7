#ifndef _FIND_DIRECTORY_H
#define _FIND_DIRECTORY_H
#include "SupportDefs.h"
#include "Path.h"
enum directory_which { B_USER_SETTINGS_DIRECTORY, B_SYSTEM_TEMP_DIRECTORY };
status_t find_directory(directory_which which, BPath* path);
#endif
