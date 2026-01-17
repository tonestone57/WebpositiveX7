#ifndef FIND_DIRECTORY_H
#define FIND_DIRECTORY_H

#include <SupportDefs.h>

enum directory_which {
	B_USER_SETTINGS_DIRECTORY
};

class BPath;

status_t find_directory(directory_which which, BPath* path);

#endif
