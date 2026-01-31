#ifndef _MOCK_LOOPER_H
#define _MOCK_LOOPER_H
#include "Handler.h"

class BLooper : public BHandler {
public:
    BLooper() {}
    virtual ~BLooper() {}
};
#endif
