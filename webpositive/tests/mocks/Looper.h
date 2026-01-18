#ifndef LOOPER_H
#define LOOPER_H

#include <Handler.h>

class BLooper : public BHandler {
public:
    BLooper(const char* name = NULL) : BHandler(name) {}
};

#endif
